// ++RED
#ifndef __TRACYDYNAMIC_HPP__
#define __TRACYDYNAMIC_HPP__

#ifndef TRACY_ENABLE

#define TracyDynamicZoneBegin(l, s, f, c, n)
#define TracyDynamicZoneEnd()
#define TracyDynamicZoneText(t)
#define TracyDynamicZonePseudo(line, source, func, color, name, startNs, endNs)
#define TracyDynamicZoneName(n)
#define TracyDynamicMessage(msg)

#include <string.h>
#else

#include <assert.h>

#include "common/TracyColor.hpp"
#include "common/TracyAlign.hpp"
#include "common/TracyForceInline.hpp"
#include "common/TracySystem.hpp"
#include "client/TracyProfiler.hpp"

#define TracyDynamicZoneBegin(line, source, func, color, name)                  tracy::detail::I_TracyDynamicZoneBegin(line,source,func,color, name, tracy::Profiler::GetTime())
#define TracyDynamicZoneEnd()                                                   tracy::detail::I_TracyDynamicZoneEnd(tracy::Profiler::GetTime())
#define TracyDynamicZonePseudo(line, source, func, color, name, startNs, endNs) tracy::detail::I_TracyDynamicZoneBegin(line,source,func,color, name, startNs); tracy::detail::I_TracyDynamicZoneEnd(endNs)
#define TracyDynamicZoneText(text)                                              tracy::detail::I_TracyDynamicZoneText(text)
#define TracyDynamicZoneName(name)                                              tracy::detail::I_TracyDynamicZoneName(name)
#define TracyDynamicMessage(msg)                                                tracy::detail::I_TracyDynamicMessage(msg)

namespace tracy
{

#ifdef TRACY_ON_DEMAND
    TRACY_API LuaZoneState& GetLuaZoneState();
#endif

    namespace detail
    {

        static inline void I_TracyDynamicZoneBegin(uint32_t line, const char* source, const char* func, uint32_t color, const char* name, int64_t time)
        {
#ifdef TRACY_ON_DEMAND
            const auto zoneCnt = GetLuaZoneState().counter++;
            if (zoneCnt != 0 && !GetLuaZoneState().active) return;
            GetLuaZoneState().active = GetProfiler().IsConnected();
            if (!GetLuaZoneState().active) return;
#endif

            //const  = Color::DeepSkyBlue3;

            const auto nsz = strlen(name);
            const auto fsz = strlen(func);
            const auto ssz = strlen(source);

            // Data layout:
            //  4b  payload size
            //  4b  color
            //  4b  source line
            //  fsz function name
            //  1b  null terminator
            //  ssz source file name
            //  1b  null terminator
            //  nsz zone name
            const uint32_t sz = uint32_t(4 + 4 + 4 + fsz + 1 + ssz + 1 + nsz);
            auto ptr = (char*)tracy_malloc(sz);
            memcpy(ptr, &sz, 4);
            memcpy(ptr + 4, &color, 4);
            memcpy(ptr + 8, &line, 4);
            memcpy(ptr + 12, func, fsz + 1);
            memcpy(ptr + 12 + fsz + 1, source, ssz + 1);
            memcpy(ptr + 12 + fsz + 1 + ssz + 1, name, nsz);

            Magic magic;
            auto token = GetToken();
            auto& tail = token->get_tail_index();
            auto item = token->enqueue_begin(magic);
            MemWrite(&item->hdr.type, QueueType::ZoneBeginAllocSrcLocCallstack);
            MemWrite(&item->zoneBegin.time, time);
            MemWrite(&item->zoneBegin.srcloc, (uint64_t)ptr);
            tail.store(magic + 1, std::memory_order_release);


            return;
        }
#endif


        static inline void I_TracyDynamicZoneEnd(int64_t time)
        {
#ifdef TRACY_ON_DEMAND
            assert(GetLuaZoneState().counter != 0);
            GetLuaZoneState().counter--;
            if (!GetLuaZoneState().active) return;
            if (!GetProfiler().IsConnected())
            {
                GetLuaZoneState().active = false;
                return;
            }
#endif

            Magic magic;
            auto token = GetToken();
            auto& tail = token->get_tail_index();
            auto item = token->enqueue_begin(magic);
            MemWrite(&item->hdr.type, QueueType::ZoneEnd);
            MemWrite(&item->zoneEnd.time, time);
            tail.store(magic + 1, std::memory_order_release);
            return;
        }

        static inline void I_TracyDynamicZonePseudo(uint32_t line, const char* source, const char* func, const char* name, int64_t startOffsetNs, int64_t endOffsetNs)
        {
        }

        static inline void I_TracyDynamicZoneText(const char* txt)
        {
#ifdef TRACY_ON_DEMAND
            if (!GetLuaZoneState().active) return;
            if (!GetProfiler().IsConnected())
            {
                GetLuaZoneState().active = false;
                return;
            }
#endif

            const auto size = strlen(txt);

            Magic magic;
            auto token = GetToken();
            auto ptr = (char*)tracy_malloc(size + 1);
            memcpy(ptr, txt, size);
            ptr[size] = '\0';
            auto& tail = token->get_tail_index();
            auto item = token->enqueue_begin(magic);
            MemWrite(&item->hdr.type, QueueType::ZoneText);
            MemWrite(&item->zoneText.text, (uint64_t)ptr);
            tail.store(magic + 1, std::memory_order_release);
            return;
        }

        static inline void I_TracyDynamicZoneName(const char* txt)
        {
#ifdef TRACY_ON_DEMAND
            if (!GetLuaZoneState().active) return;
            if (!GetProfiler().IsConnected())
            {
                GetLuaZoneState().active = false;
                return;
            }
#endif

            const auto size = strlen(txt);

            Magic magic;
            auto token = GetToken();
            auto ptr = (char*)tracy_malloc(size + 1);
            memcpy(ptr, txt, size);
            ptr[size] = '\0';
            auto& tail = token->get_tail_index();
            auto item = token->enqueue_begin(magic);
            MemWrite(&item->hdr.type, QueueType::ZoneName);
            MemWrite(&item->zoneText.text, (uint64_t)ptr);
            tail.store(magic + 1, std::memory_order_release);
            return;
        }

        static inline void I_TracyDynamicMessage(const char* txt)
        {
#ifdef TRACY_ON_DEMAND
            if (!GetProfiler().IsConnected()) return;
#endif

            const auto size = strlen(txt);

            Magic magic;
            auto token = GetToken();
            auto ptr = (char*)tracy_malloc(size + 1);
            memcpy(ptr, txt, size);
            ptr[size] = '\0';
            auto& tail = token->get_tail_index();
            auto item = token->enqueue_begin(magic);
            MemWrite(&item->hdr.type, QueueType::Message);
            MemWrite(&item->message.time, Profiler::GetTime());
            MemWrite(&item->message.text, (uint64_t)ptr);
            tail.store(magic + 1, std::memory_order_release);
            return;
        }

    }
};

#endif
// --RED