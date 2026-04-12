# Filename: BuildOptionsLinux.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

#
# Adds Linux and generic Unix build options that are intentionally user-controlled.
# The caller passes variable names so this helper can append into local accumulators.
#
function(lugizmo_df_apply_linux_build_options CXX_FLAGS_VAR DEFINITIONS_VAR)

    # ====== POSITION INDEPENDENT CODE =================================================================================

    if(LUGIZMO_DF_ENABLE_PIC)
        _lugizmo_df_add_flag_if_supported(${CXX_FLAGS_VAR} -fPIC)
    endif()

    # ====== HARDENING =================================================================================================

    # TODO add linux hardening

    set(${CXX_FLAGS_VAR} "${${CXX_FLAGS_VAR}}" PARENT_SCOPE)
    set(${DEFINITIONS_VAR} "${${DEFINITIONS_VAR}}" PARENT_SCOPE)
endfunction()
