cmake_minimum_required(VERSION 3.22.0)

function(prj_register_var var_name var_desciption var_default)
  set(${LOCAL_PROJECT_NAME_CAPS}_${var_name} ${var_default} CACHE STRING ${var_desciption})
  set("PRJ_VAR_${var_name}" "${${LOCAL_PROJECT_NAME_CAPS}_${var_name}}" PARENT_SCOPE)
  # Add a global compile definition for the variable.
  add_compile_definitions("${LOCAL_PROJECT_NAME_CAPS}_${var_name}=\"${${LOCAL_PROJECT_NAME_CAPS}_${var_name}}\"")
  set(PRJ_CUSTOM_VARIABLES ${PRJ_CUSTOM_VARIABLES} "${LOCAL_PROJECT_NAME_CAPS}_${var_name}=${${LOCAL_PROJECT_NAME_CAPS}_${var_name}}" PARENT_SCOPE)
endfunction()

function(prj_register_option option_name option_desciption option_default)
  option("${LOCAL_PROJECT_NAME_CAPS}_${option_name}" ${option_desciption} ${option_default})
  set("PRJ_OPT_${option_name}" "${${LOCAL_PROJECT_NAME_CAPS}_${option_name}}" PARENT_SCOPE)
  if("${${LOCAL_PROJECT_NAME_CAPS}_${option_name}}")
    # Add a global compile definition for the option.
    add_compile_definitions("${LOCAL_PROJECT_NAME_CAPS}_${option_name}" "${PRJ_OPT_${option_name}}")
    set(PRJ_ENABLED_OPTIONS ${PRJ_ENABLED_OPTIONS} "${LOCAL_PROJECT_NAME_CAPS}_${option_name}" PARENT_SCOPE)
  endif()
endfunction()
