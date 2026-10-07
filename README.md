# BASIC_2026
Simple BASIC interpreter, eventually for embedded use.

Simple BASIC interpreter, written in C.  Intended to eventually be used on embedded systems (shades of Intel BASIC-52.)

Data Types:
  integer (32 bit).
  float (64 bit IEEE double precision.)
  string

Statements:
  LET
  IF THEN ELSE
  GOTO
  GOSUB
  RETURN
  FOR TO STEP
  NEXT
  INPUT
  PRINT
  DIM
  END

Arithmetic Operators:
Some borrowed from C.  All have precedence same as equivalent C operators.

*, / , %   Multiplication, Division, Modulus
+, -       Addition, Subtraction


AND        Logical AND (short circuit)
OR         Logical OR (short circuit)

