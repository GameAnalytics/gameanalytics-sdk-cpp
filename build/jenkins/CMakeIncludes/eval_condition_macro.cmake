# Evaluates condition and sets variable

MACRO(EVAL_CONDITION name)
   IF(${ARGN})
     SET(${name} 1)
   ELSE()
     SET(${name} 0)
   ENDIF()
ENDMACRO(EVAL_CONDITION)
