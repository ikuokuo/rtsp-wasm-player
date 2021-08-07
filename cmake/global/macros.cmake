# msg_var(variableName)
macro(msg_var variableName)
  message(STATUS "${variableName}: ${${variableName}}")
endmacro()
