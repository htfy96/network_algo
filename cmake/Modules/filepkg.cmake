#   Copyright 2016 Vic Luo (vicluo96@gmail.com)
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

if (FILEPKG_DONE)
    return()
endif()

set(FILEPKG_DONE ON)

set(INSTALL_FILE_AUTOCONFIRM OFF CACHE BOOL)

function(whiptail_select_list_ msg list_var)
    set(cmd_line_args "--radiolist" "--clear" "${msg}" 30 40 20)
    set(first_item 1)
    set(counter 0)
    set(list_var ${list_var})
    foreach(item IN LISTS list_var)
        list(APPEND cmd_line_args ${counter} ${item} ${first_item})
        set(first_item 0)
        math(EXPR counter "${counter} + 1")
    endforeach()
    execute_process(COMMAND whiptail ${cmd_line_args} ERROR_VARIABLE list_idx RESULT_VARIABLE list_result)
    list(GET list_var ${list_idx} list_choosen)
    set(list_choosen ${list_choosen} PARENT_SCOPE)
endfunction(whiptail_select_list_)

function(pacmac_install_pkg_ pkg)
    message(STATUS "Installing package ${pkg}")
    if (INSTALL_FILE_AUTOCONFIRM)
        set(autoconfirmcmd --noconfirm)
    endiF()

    execute_process(COMMAND sudo pacman -S ${autoconfirmcmd} --needed ${pkg} RESULT_VARIABLE install_failed)
    if (install_failed)
        message(WARNING "${pkg} install failed")
        set(INSTALL_FILE_FAILED 1 PARENT_SCOPE)
        return()
    else()
        message(STATUS "${pkg} install succeeded")
    endif()
endfunction(pacmac_install_pkg_)

function(prepare_pacman_)
    find_program(pkgfile_file pkgfile)
    message(STATUS "pkgfile location: ${pkgfile_file}")
    if (pkgfile_file STREQUAL pkgfile_file-NOTFOUND)
        pacmac_install_pkg_(pkgfile)

        message(STATUS "updating pkgfile cache")
        execute_process(COMMAND sudo pkgfile --update RESULT_VARIABLE pkgfile_update_failed)
        if (pkgfile_update_failed)
            message(WARNING "pkgfile update failed")
            set(INSTALL_FILE_FAILED 1 PARENT_SCOPE)
            return()
        endif()
    endif()

    find_program(whiptail_file whiptail)
    message(STATUS "whiptail location: ${whiptail_file}")
    if (whiptail_file STREQUAL whiptail_file-NOTFOUND)
        pacmac_install_pkg_(libnewt)
    endif()
    if (INSTALL_FILE_FAILED)
        return()
    endif()
    message(STATUS "whiptail is ready!")
endfunction(prepare_pacman_)

function(pacman_find_and_install_ file_pattern)
    message(STATUS "finding file: ${file_pattern}")
    message(STATUS "PKGCMD: ${pkgfile_file} -g ${file_pattern}")
    execute_process(COMMAND ${pkgfile_file} -g *${file_pattern} RESULT_VARIABLE pkg_result OUTPUT_VARIABLE pkg_string ERROR_VARIABLE pkg_error OUTPUT_STRIP_TRAILING_WHITESPACE)
    if ("${pkg_string}" STREQUAL "")
        message(WARNING "requested package ${file_pattern} not found!")
        set(INSTALL_FILE_FAILED 1 PARENT_SCOPE)
        return()
    endif()
    STRING(REGEX REPLACE "\n" ";" pkg_list ${pkg_string})
    message(STATUS "Candidate pkg list: ${pkg_list}")

    whiptail_select_list_("Please select preferred package providing file pattern ${file_pattern} using spacebar and then enter" "${pkg_list}")

    set(candidate_pkgname ${list_choosen})

    pacmac_install_pkg_(${candidate_pkgname})
    if (INSTALL_FILE_FAILED)
        return()
    endif()

    message(STATUS "Successfully installed ${candidate_pkgname} from pattern ${file_pattern}")
endfunction(pacman_find_and_install_)

# APT
function(apt_install_pkg_ pkg)
    message(STATUS "Installing package ${pkg}")
    if (INSTALL_FILE_AUTOCONFIRM)
        set(autoconfirmcmd -y)
    endiF()

    execute_process(COMMAND sudo apt-get install ${autoconfirmcmd} ${pkg} RESULT_VARIABLE install_failed)
    if (install_failed)
        message(WARNING "${pkg} install failed")
        set(INSTALL_FILE_FAILED 1 PARENT_SCOPE)
        return()
    else()
        message(STATUS "${pkg} install succeeded")
    endif()
endfunction(apt_install_pkg_)

function(prepare_apt_)
    find_program(apt-file_file apt-file)
    message(STATUS "apt-file location: ${apt-file_file}")
    if (apt-file_file STREQUAL apt-file_file-NOTFOUND)
        apt_install_pkg_(apt-file)

        message(STATUS "updating apt-file cache")
        execute_process(COMMAND sudo apt-file update RESULT_VARIABLE apt-file_update_failed)
        if (apt-file_update_failed)
            message(WARNING "apt-file update failed")
            set(INSTALL_FILE_FAILED 1 PARENT_SCOPE)
            return()
        endif()
    endif()

    find_program(whiptail_file whiptail)
    message(STATUS "whiptail location: ${whiptail_file}")
    if (whiptail_file STREQUAL whiptail_file-NOTFOUND)
        pacmac_install_pkg_(whiptail)
    endif()
    if (INSTALL_FILE_FAILED)
        return()
    endif()
    message(STATUS "whiptail is ready!")
endfunction(prepare_apt_)

function(apt_find_and_install_ file_pattern)
    message(STATUS "finding file: ${file_pattern}")
    message(STATUS "aptcmd: apt-file search ${file_pattern}")
    execute_process(COMMAND apt-file search ${file_pattern} --package-only RESULT_VARIABLE pkg_result OUTPUT_VARIABLE pkg_string ERROR_VARIABLE pkg_error OUTPUT_STRIP_TRAILING_WHITESPACE)
    if ("${pkg_string}" STREQUAL "")
        message(WARNING "requested package ${file_pattern} not found!")
        set(INSTALL_FILE_FAILED 1 PARENT_SCOPE)
        return()
    endif()
    STRING(REGEX REPLACE "\n" ";" pkg_list ${pkg_string})
    message(STATUS "Candidate pkg list: ${pkg_list}")

    whiptail_select_list_("Please select preferred package providing file pattern ${file_pattern} using spacebar and then enter" "${pkg_list}")

    set(candidate_pkgname ${list_choosen})

    apt_install_pkg_(${candidate_pkgname})
    if (INSTALL_FILE_FAILED)
        return()
    endif()

    message(STATUS "Successfully installed ${candidate_pkgname} from pattern ${file_pattern}")
endfunction(apt_find_and_install_)

function(INSTALL_FILE file_pattern)
    set(INSTALL_FILE_FAILED 0 PARENT_SCOPE)
    find_program(pacman_file pacman)
    message(STATUS "pacman location: ${pacman_file}")
    if (NOT pacman_file STREQUAL pacman_file-NOTFOUND)
        message(STATUS "pacman Found!")
        prepare_pacman_()
        if (INSTALL_FILE_FAILED)
            return()
        endif()
        pacman_find_and_install_(${file_pattern})
        if (INSTALL_FILE_FAILED)
            return()
        endif()
    endif()

    find_program(aptget_file apt-get)
    message(STATUS "apt-get location: ${aptget_file}")
    if (NOT aptget_file STREQUAL aptget_file-NOTFOUND)
        message(STATUS "apt-get found")
        prepare_apt_()
        if (INSTALL_FILE_FAILED)
            return()
        endif()
        apt_find_and_install_(${file_pattern})
        if (INSTALL_FILE_FAILED)
            return()
        endif()
    endif()

    message(STATUS "Succeeded to install file pattern ${file_pattern}")
endfunction(INSTALL_FILE)
