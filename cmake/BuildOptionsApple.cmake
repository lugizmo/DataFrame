# Filename: BuildOptionsApple.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

#
# Adds Apple-specific hardening, libc++ selection, and debug-safety flags.
# The caller passes variable names so this helper can append into local accumulators.
#
function(lugizmo_df_apply_apple_build_options CXX_FLAGS_VAR DEFINITIONS_VAR)
    set(LUGIZMO_DF_NOT_DEBUG_OR_RELWITHDEBINFO "$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>")

    # ====== COMMON ====================================================================================================

    if(LUGIZMO_DF_STRICT_MODE)
        # Strict mode uses libc++ consistently across all configurations.
        _lugizmo_df_add_flag_if_supported(${CXX_FLAGS_VAR} -stdlib=libc++)
    else()
        # Non-strict mode only forces libc++ for Debug where the local Apple
        # diagnostics and hardening policy is most relevant.
        _lugizmo_df_add_flag_if_supported_for_cond(${CXX_FLAGS_VAR} -stdlib=libc++ "$<CONFIG:Debug>")
    endif()

    # ====== HARDENING =================================================================================================

    if(LUGIZMO_DF_ENABLE_HARDENING)
        if(NOT LUGIZMO_DF_STRICT_MODE)
            # Fast debug hardening keeps the debugger experience smoother while
            # still enabling stack protection and safer libc++ checks.
            _lugizmo_df_add_flag_if_supported_for_cond(${CXX_FLAGS_VAR} -fstack-protector-strong "$<CONFIG:Debug>")
            list(APPEND ${DEFINITIONS_VAR} "$<$<CONFIG:Debug>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST>")
        else()
            # Strict debug mode prefers maximum diagnostics and stronger runtime
            # hardening over build speed and debugger convenience.
            foreach(FLAG -fstrict-enums -fstack-protector-all -fno-omit-frame-pointer -fno-optimize-sibling-calls)
                _lugizmo_df_add_flag_if_supported_for_cond(${CXX_FLAGS_VAR} "${FLAG}" "$<CONFIG:Debug>")
            endforeach()

            # RelWithDebInfo keeps an optimized build, so only the lighter stack
            # hardening and the more extensive libc++ checks are added here.
            _lugizmo_df_add_flag_if_supported_for_cond(${CXX_FLAGS_VAR} -fstack-protector-strong "$<CONFIG:RelWithDebInfo>")

            # Release keeps the runtime checks light while retaining baseline
            # hardening suitable for optimized builds.
            _lugizmo_df_add_flag_if_supported_for_cond(${CXX_FLAGS_VAR} -fstack-protector-strong "${LUGIZMO_DF_NOT_DEBUG_OR_RELWITHDEBINFO}")

            list(APPEND ${DEFINITIONS_VAR}
                 "$<$<CONFIG:Debug>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG>"
                 "$<$<CONFIG:RelWithDebInfo>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE>"
                 "$<${LUGIZMO_DF_NOT_DEBUG_OR_RELWITHDEBINFO}:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST>")
        endif()
    endif()

    set(${CXX_FLAGS_VAR} "${${CXX_FLAGS_VAR}}" PARENT_SCOPE)
    set(${DEFINITIONS_VAR} "${${DEFINITIONS_VAR}}" PARENT_SCOPE)
endfunction()
