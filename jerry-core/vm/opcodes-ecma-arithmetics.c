/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecma-alloc.h"
#include "ecma-conversion.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-try-catch-macro.h"
#include "opcodes.h"
#include "jrt-libc-includes.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * Perform ECMA number arithmetic operation.
 *
 * The algorithm of the operation is following:
 *   leftNum = ToNumber (leftValue);
 *   rightNum = ToNumber (rightValue);
 *   result = leftNum ArithmeticOp rightNum;
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
do_number_arithmetic (number_arithmetic_op op, /**< number arithmetic operation */
                      ecma_value_t left_value, /**< left value */
                      ecma_value_t right_value) /**< right value */
{
  JERRY_STATIC_ASSERT (ECMA_INTEGER_MULTIPLY_MAX * ECMA_INTEGER_MULTIPLY_MAX <= ECMA_INTEGER_NUMBER_MAX
                       && -(ECMA_INTEGER_MULTIPLY_MAX * ECMA_INTEGER_MULTIPLY_MAX) >= ECMA_INTEGER_NUMBER_MIN,
                       square_of_integer_multiply_max_must_fit_into_integer_value_range);
  JERRY_STATIC_ASSERT (ECMA_INTEGER_NUMBER_MAX * 2 <= INT32_MAX
                       && ECMA_INTEGER_NUMBER_MIN * 2 >= INT32_MIN,
                       doubled_ecma_numbers_must_fit_into_int32_range);

  JERRY_ASSERT (!ecma_is_value_error (left_value)
                && !ecma_is_value_error (right_value));

  if (ecma_are_values_integer_numbers (left_value, right_value))
  {
    switch (op)
    {
      case NUMBER_ARITHMETIC_ADDITION:
      {
        ecma_integer_value_t left_integer = ecma_get_integer_from_value (left_value);
        ecma_integer_value_t right_integer = ecma_get_integer_from_value (right_value);
        return ecma_make_int32_value ((int32_t) (left_integer + right_integer));
      }
      case NUMBER_ARITHMETIC_SUBSTRACTION:
      {
        ecma_integer_value_t left_integer = ecma_get_integer_from_value (left_value);
        ecma_integer_value_t right_integer = ecma_get_integer_from_value (right_value);
        return ecma_make_int32_value ((int32_t) (left_integer - right_integer));
      }
      case NUMBER_ARITHMETIC_MULTIPLICATION:
      {
        ecma_integer_value_t left_integer = ecma_get_integer_from_value (left_value);
        ecma_integer_value_t right_integer = ecma_get_integer_from_value (right_value);

        if (-ECMA_INTEGER_MULTIPLY_MAX <= left_integer
            && left_integer <= ECMA_INTEGER_MULTIPLY_MAX
            && -ECMA_INTEGER_MULTIPLY_MAX <= right_integer
            && right_integer <= ECMA_INTEGER_MULTIPLY_MAX)
        {
          return ecma_make_integer_value (left_integer * right_integer);
        }
        break;
      }
      case NUMBER_ARITHMETIC_DIVISION:
      {
        /* Not optimized since the result is likely not an integer number. */
        break;
      }
      case NUMBER_ARITHMETIC_REMAINDER:
      {
        ecma_integer_value_t left_integer = ecma_get_integer_from_value (left_value);
        ecma_integer_value_t right_integer = ecma_get_integer_from_value (right_value);
        if (right_integer != 0)
        {
          ecma_integer_value_t result = left_integer % right_integer;

          if (result != 0 || left_integer >= 0)
          {
            return ecma_make_integer_value (result);
          }
        }
        break;
      }
    }
  }

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_left, left_value, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_right, right_value, ret_value);

  ecma_number_t result = ECMA_NUMBER_ZERO;

  switch (op)
  {
    case NUMBER_ARITHMETIC_ADDITION:
    {
      result = ecma_number_add (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_SUBSTRACTION:
    {
      result = ecma_number_substract (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_MULTIPLICATION:
    {
      result = ecma_number_multiply (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_DIVISION:
    {
      result = ecma_number_divide (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_REMAINDER:
    {
      result = ecma_op_number_remainder (num_left, num_right);
      break;
    }
  }

  ret_value = ecma_make_number_value (result);

  ECMA_OP_TO_NUMBER_FINALIZE (num_right);
  ECMA_OP_TO_NUMBER_FINALIZE (num_left);

  return ret_value;
} /* do_number_arithmetic */

/**
 * 'Addition' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_addition (ecma_value_t left_value, /**< left value */
                 ecma_value_t right_value) /**< right value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (prim_left_value,
                  ecma_op_to_primitive (left_value,
                                        ECMA_PREFERRED_TYPE_NO),
                  ret_value);
  ECMA_TRY_CATCH (prim_right_value,
                  ecma_op_to_primitive (right_value,
                                        ECMA_PREFERRED_TYPE_NO),
                  ret_value);

  if (ecma_is_value_string (prim_left_value)
      || ecma_is_value_string (prim_right_value))
  {
    ECMA_TRY_CATCH (str_left_value, ecma_op_to_string (prim_left_value), ret_value);
    ECMA_TRY_CATCH (str_right_value, ecma_op_to_string (prim_right_value), ret_value);

    ecma_string_t *string1_p = ecma_get_string_from_value (str_left_value);
    ecma_string_t *string2_p = ecma_get_string_from_value (str_right_value);

    ecma_string_t *concat_str_p = ecma_concat_ecma_strings (string1_p, string2_p);

    ret_value = ecma_make_string_value (concat_str_p);

    ECMA_FINALIZE (str_right_value);
    ECMA_FINALIZE (str_left_value);
  }
  else
  {
    ret_value = do_number_arithmetic (NUMBER_ARITHMETIC_ADDITION,
                                      left_value,
                                      right_value);
  }

  ECMA_FINALIZE (prim_right_value);
  ECMA_FINALIZE (prim_left_value);

  return ret_value;
} /* opfunc_addition */

/**
 * 'Unary "+"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_unary_plus (ecma_value_t left_value) /**< left value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               left_value,
                               ret_value);

  ret_value = ecma_make_number_value (num_var_value);

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);

  return ret_value;
} /* opfunc_unary_plus */

/**
 * 'Unary "-"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_unary_minus (ecma_value_t left_value) /**< left value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               left_value,
                               ret_value);

  ret_value = ecma_make_number_value (ecma_number_negate (num_var_value));

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);

  return ret_value;
} /* opfunc_unary_minus */

/**
 * @}
 * @}
 */
