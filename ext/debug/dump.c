/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (debug://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  |          ZhuZongXin <dreamsxin@qq.com>                                 |
  +------------------------------------------------------------------------+
*/

#include "debug/dump.h"
#include "debug/exception.h"

#include "kernel/main.h"
#include "kernel/exception.h"
#include "kernel/memory.h"
#include "kernel/fcall.h"
#include "kernel/object.h"
#include "kernel/hash.h"
#include "kernel/array.h"
#include "kernel/concat.h"
#include "kernel/operators.h"
#include "kernel/string.h"

/**
 * Phalcon\Debug\Dump
 *
 * Dumps information about a variable(s)
 *
 * <code>
 *    $foo = 123;
 *    echo (new \Phalcon\Debug\Dump())->variable($foo, "foo");
 *</code>
 *
 * <code>
 *    $foo = "string";
 *    $bar = ["key" => "value"];
 *    $baz = new stdClass();
 *    echo (new \Phalcon\Debug\Dump())->variables($foo, $bar, $baz);
 *</code>
 */
zend_class_entry *phalcon_debug_dump_ce;

PHP_METHOD(Phalcon_Debug_Dump, __construct);
PHP_METHOD(Phalcon_Debug_Dump, all);
PHP_METHOD(Phalcon_Debug_Dump, getStyle);
PHP_METHOD(Phalcon_Debug_Dump, setStyles);
PHP_METHOD(Phalcon_Debug_Dump, output);
PHP_METHOD(Phalcon_Debug_Dump, variable);
PHP_METHOD(Phalcon_Debug_Dump, variables);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_debug_dump___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, styles)
	ZEND_ARG_INFO(0, detailed)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_debug_dump_getstyle, 0, 0, 1)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_debug_dump_setstyles, 0, 0, 0)
	ZEND_ARG_INFO(0, styles)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_debug_dump_output, 0, 0, 1)
	ZEND_ARG_INFO(0, variable)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, tab)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_debug_dump_variable, 0, 0, 1)
	ZEND_ARG_INFO(0, variable)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_debug_dump_method_entry[] = {
	PHP_ME(Phalcon_Debug_Dump, __construct, arginfo_phalcon_debug_dump___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Debug_Dump, all, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Debug_Dump, getStyle, arginfo_phalcon_debug_dump_getstyle, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Debug_Dump, setStyles, arginfo_phalcon_debug_dump_setstyles, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Debug_Dump, output, arginfo_phalcon_debug_dump_output, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Debug_Dump, variable, arginfo_phalcon_debug_dump_variable, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Debug_Dump, variables, NULL, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Debug_Dump, one, variable, arginfo_phalcon_debug_dump_variable, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Debug_Dump, var, variable, arginfo_phalcon_debug_dump_variable, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Debug_Dump, vars, variables, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Debug\Dump initializer
 */
PHALCON_INIT_CLASS(Phalcon_Debug_Dump){

	PHALCON_REGISTER_CLASS(Phalcon\\Debug, Dump, debug_dump, phalcon_debug_dump_method_entry, 0);

	zend_declare_property_bool(phalcon_debug_dump_ce, SL("_detailed"), 0, ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_debug_dump_ce, SL("_styles"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_debug_dump_ce, SL("_objects"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}

/**
 * Phalcon\Debug\Dump constructor
 *
 * @param array styles set styles for vars type
 * @param boolean detailed debug object's private and protected properties
 */
PHP_METHOD(Phalcon_Debug_Dump, __construct){

	zval *styles = NULL, *detailed = NULL, *default_styles;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 2, &styles, &detailed);

	PHALCON_INIT_VAR(default_styles);
	array_init(default_styles);

	phalcon_array_update_str_str(default_styles, SL("pre"), SL("background-color:#f3f3f3; font-size:11px; padding:10px; border:1px solid #ccc; text-align:left; color:#333"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("arr"), SL("color:red"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("bool"), SL("color:green"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("float"), SL("color:fuchsia"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("int"), SL("color:blue"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("null"), SL("color:black"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("num"), SL("color:navy"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("obj"), SL("color:purple"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("other"), SL("color:maroon"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("res"), SL("color:lime"), PH_COPY);
	phalcon_array_update_str_str(default_styles, SL("str"), SL("color:teal"), PH_COPY);

	phalcon_update_property_this(getThis(), SL("_styles"), default_styles);

	if (styles && Z_TYPE_P(styles) != IS_NULL) {
		PHALCON_CALL_SELF(NULL, "setstyles", styles);
	}

	if (detailed && zend_is_true(detailed)) {
		phalcon_update_property_bool(getThis(), SL("_detailed"), 1);
	}

	phalcon_update_property_empty_array(getThis(), SL("_objects"));

	PHALCON_MM_RESTORE();
}

/**
 * Alias of variables() method
 *
 * @param mixed variable
 * @param ...
 */
PHP_METHOD(Phalcon_Debug_Dump, all){

	zval *method_name, *call_object, *arg_list = NULL;

	PHALCON_MM_GROW();

	PHALCON_INIT_VAR(method_name);
	ZVAL_STRING(method_name, "variables");

	PHALCON_INIT_VAR(call_object);
	array_init_size(call_object, 2);
	phalcon_array_append(call_object, getThis(), PH_COPY);
	phalcon_array_append(call_object, method_name, PH_COPY);

	PHALCON_CALL_FUNCTION(&arg_list, "func_get_args");

	PHALCON_CALL_USER_FUNC_ARRAY(&return_value, call_object, arg_list);

	RETURN_MM();
}

/**
 * Get style for type
 */
PHP_METHOD(Phalcon_Debug_Dump, getStyle){

	zval *type, *styles, *style;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &type);

	styles  = phalcon_read_property(getThis(), SL("_styles"), PH_NOISY);

	if (phalcon_array_isset(styles, type)) {
		PHALCON_OBS_VAR(style);
		phalcon_array_fetch(&style, styles, type, PH_NOISY);
	} else {
		PHALCON_INIT_VAR(style);
		ZVAL_STRING(style, "color:gray");
	}

	RETURN_CTOR(style);
}

/**
 * Set styles for vars type
 */
PHP_METHOD(Phalcon_Debug_Dump, setStyles){

	zval *styles, *default_styles, *new_styles;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &styles);

	if (Z_TYPE_P(styles) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_debug_exception_ce, "The styles must be an array");
		return;
	}

	default_styles  = phalcon_read_property(getThis(), SL("_styles"), PH_NOISY);

	PHALCON_INIT_VAR(new_styles);
	phalcon_fast_array_merge(new_styles, default_styles, styles);

	phalcon_update_property_this(getThis(), SL("_styles"), new_styles);

	RETURN_THIS();
}

/**
 * Prepare an HTML string of information about a single variable.
 */
PHP_METHOD(Phalcon_Debug_Dump, output){

	zval *variable, *name = NULL, *tab = NULL, *space, *tmp = NULL, *new_tab = NULL;
	zval *output = NULL, *str = NULL, *type = NULL, *style = NULL, *count = NULL, *value = NULL, *replace_pairs = NULL;
	zval *class_name = NULL, *objects, *detailed = NULL, *properties = NULL, *methods = NULL, *method = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 2, &variable, &name, &tab);

	if (!name) {
		name = &PHALCON_GLOBAL(z_null);
	} else if (!PHALCON_IS_EMPTY(name)) {
		PHALCON_CONCAT_SVS(return_value, "var ", name, " ");
	}

	if (!tab) {
		tab = &PHALCON_GLOBAL(z_one);
	}

	PHALCON_INIT_VAR(space);
	ZVAL_STRING(space, "  ");

	if (Z_TYPE_P(variable) == IS_ARRAY) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style =':style'>Array</b> (<span style =':style'>:count</span>) (\n");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "arr");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(count);
		ZVAL_LONG(count, phalcon_fast_count_int(variable));

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":count"), count, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		phalcon_concat_self(return_value, output);

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(variable), idx, str_key, value) {
			zval key;
			if (str_key) {
				ZVAL_STR(&key, str_key);
			} else {
				ZVAL_LONG(&key, idx);
			}

			PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

			phalcon_concat_self(return_value, tmp);

			PHALCON_INIT_NVAR(str);
			ZVAL_STRING(str, "[<span style=':style'>:key</span>] => ");

			PHALCON_INIT_NVAR(type);
			ZVAL_STRING(type, "arr");

			PHALCON_CALL_SELF(&style, "getstyle", type);

			PHALCON_INIT_NVAR(replace_pairs);
			array_init(replace_pairs);

			phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
			phalcon_array_update_str(replace_pairs, SL(":key"), &key, PH_COPY);

			PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

			phalcon_concat_self(return_value, output);

			if (PHALCON_IS_LONG(tab, 1) && !PHALCON_IS_EMPTY(name) && !phalcon_is_numeric(&key) && PHALCON_IS_IDENTICAL(name, &key)) {
				continue;
			} else {
				PHALCON_INIT_NVAR(new_tab);
				ZVAL_LONG(new_tab, Z_LVAL_P(tab) + 1);

				PHALCON_CALL_SELF(&tmp, "output", value, &PHALCON_GLOBAL(z_null), new_tab);
				PHALCON_SCONCAT_VS(return_value, tmp, "\n");
			}
		} ZEND_HASH_FOREACH_END();

		PHALCON_INIT_NVAR(new_tab);
		ZVAL_LONG(new_tab, Z_LVAL_P(tab) - 1);

		PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

		PHALCON_SCONCAT(return_value, tmp);
	} else if (Z_TYPE_P(variable) == IS_OBJECT) {

		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>Object</b> :class");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "obj");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(class_name);
		phalcon_get_class(class_name, variable, 0);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":class"), class_name, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);

		PHALCON_INIT_NVAR(class_name);
		phalcon_get_parent_class(class_name, variable, 0);

		if (zend_is_true(class_name)) {
			PHALCON_INIT_NVAR(str);
			ZVAL_STRING(str, " <b style=':style'>extends</b> :parent");

			PHALCON_INIT_NVAR(type);
			ZVAL_STRING(type, "obj");

			PHALCON_CALL_SELF(&style, "getstyle", type);

			PHALCON_INIT_NVAR(replace_pairs);
			array_init(replace_pairs);

			phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
			phalcon_array_update_str(replace_pairs, SL(":parent"), class_name, PH_COPY);

			PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

			PHALCON_SCONCAT(return_value, output);
		}

		PHALCON_SCONCAT_STR(return_value, " (\n");

		objects  = phalcon_read_property(getThis(), SL("_objects"), PH_NOISY);

		if (phalcon_fast_in_array(variable, objects)) {
			
			PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);
			PHALCON_SCONCAT_VS(return_value, tmp, "[already listed]\n");

			PHALCON_INIT_NVAR(new_tab);
			ZVAL_LONG(new_tab, Z_LVAL_P(tab) - 1);

			PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

			PHALCON_SCONCAT_VS(return_value, tmp, ")");

			RETURN_MM();
		}

		phalcon_update_property_array_append(getThis(), SL("_objects"), variable);

		detailed  = phalcon_read_property(getThis(), SL("_detailed"), PH_NOISY);

		PHALCON_INIT_NVAR(properties);
		phalcon_get_object_vars(properties, variable, !zend_is_true(detailed));

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(properties), idx, str_key, value) {
			zval key;
			if (str_key) {
				ZVAL_STR(&key, str_key);
			} else {
				ZVAL_LONG(&key, idx);
			}

			PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

			PHALCON_SCONCAT(return_value, tmp);

			PHALCON_INIT_NVAR(str);
			ZVAL_STRING(str, "-><span style=':style'>:key</span> (<span style=':style'>:type</span>) = ");

			PHALCON_INIT_NVAR(type);
			ZVAL_STRING(type, "obj");

			PHALCON_CALL_SELF(&style, "getstyle", type);

			PHALCON_INIT_NVAR(replace_pairs);
			array_init(replace_pairs);

			phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
			phalcon_array_update_str(replace_pairs, SL(":key"), &key, PH_COPY);

			if (PHALCON_PROPERTY_IS_PUBLIC_ZVAL(variable, &key)) {
				phalcon_array_update_str_str(replace_pairs, SL(":type"), SL("public"), PH_COPY);
			} else if (PHALCON_PROPERTY_IS_PRIVATE_ZVAL(variable, &key)) {
				phalcon_array_update_str_str(replace_pairs, SL(":type"), SL("private"), PH_COPY);
			} else if (PHALCON_PROPERTY_IS_PROTECTED_ZVAL(variable, &key)) {
				phalcon_array_update_str_str(replace_pairs, SL(":type"), SL("protected"), PH_COPY);
			}

			PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

			PHALCON_SCONCAT(return_value, output);

			PHALCON_INIT_NVAR(new_tab);
			ZVAL_LONG(new_tab, Z_LVAL_P(tab) + 1);

			PHALCON_CALL_SELF(&tmp, "output", value, &PHALCON_GLOBAL(z_null), new_tab);
			PHALCON_SCONCAT_VS(return_value, tmp, ")\n");
		} ZEND_HASH_FOREACH_END();

		PHALCON_INIT_NVAR(methods);

		phalcon_get_class_methods(methods, variable, !zend_is_true(detailed));

		PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

		PHALCON_SCONCAT(return_value, tmp);

		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, ":class <b style=':style'>methods</b>: (<span style=':style'>:count</span>) (\n");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "obj");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(class_name);
		phalcon_get_class(class_name, variable, 0);

		PHALCON_INIT_NVAR(count);
		ZVAL_LONG(count, phalcon_fast_count_int(methods));

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":class"), class_name, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":count"), count, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(methods), method) {
			PHALCON_INIT_NVAR(new_tab);
			ZVAL_LONG(new_tab, Z_LVAL_P(tab) + 1);

			PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, new_tab);

			PHALCON_SCONCAT(return_value, tmp);

			PHALCON_INIT_NVAR(str);
			ZVAL_STRING(str, "-><span style=':style'>:method</span>();\n");

			PHALCON_INIT_NVAR(type);
			ZVAL_STRING(type, "obj");

			PHALCON_CALL_SELF(&style, "getstyle", type);

			PHALCON_INIT_NVAR(replace_pairs);
			array_init(replace_pairs);

			phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
			phalcon_array_update_str(replace_pairs, SL(":method"), method, PH_COPY);

			PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

			PHALCON_SCONCAT(return_value, output);

			PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

			PHALCON_SCONCAT_VS(return_value, tmp, "\n");
		} ZEND_HASH_FOREACH_END();

		PHALCON_INIT_NVAR(new_tab);
		ZVAL_LONG(new_tab, Z_LVAL_P(tab) - 1);

		PHALCON_CALL_FUNCTION(&tmp, "str_repeat", space, tab);

		PHALCON_SCONCAT_VS(return_value, tmp, ")");
	} else if (Z_TYPE_P(variable) == IS_LONG) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>Integer</b> (<span style=':style'>:var</span>)");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "int");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":var"), variable, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	} else if (Z_TYPE_P(variable) == IS_DOUBLE) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>Float</b> (<span style=':style'>:var</span>)");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "float");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":var"), variable, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	} else if (phalcon_is_numeric_ex(variable)) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>Numeric string</b> (<span style=':style'>:length</span>) \"<span style=':style'>:var</span>\"");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "num");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str_long(replace_pairs, SL(":length"), Z_STRLEN_P(variable), 0);
		phalcon_array_update_str(replace_pairs, SL(":var"), variable, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	} else if (Z_TYPE_P(variable) == IS_STRING) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>String</b> (<span style=':style'>:length</span>) \"<span style=':style'>:var</span>\"");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "str");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str_long(replace_pairs, SL(":length"), Z_STRLEN_P(variable), 0);
		phalcon_array_update_str(replace_pairs, SL(":var"), variable, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	} else if (PHALCON_IS_BOOL(variable)) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>Boolean</b> (<span style=':style'>:var</span>)");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "bool");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		if (zend_is_true(variable)) {
			phalcon_array_update_str_str(replace_pairs, SL(":var"), SL("TRUE") , PH_COPY);
		} else {
			phalcon_array_update_str_str(replace_pairs, SL(":var"), SL("FALSE") , PH_COPY);
		}

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	} else if (Z_TYPE_P(variable) == IS_NULL) {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "<b style=':style'>NULL</b>");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "null");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	} else {
		PHALCON_INIT_NVAR(str);
		ZVAL_STRING(str, "(<span style=':style'>:var</span>)");

		PHALCON_INIT_NVAR(type);
		ZVAL_STRING(type, "other");

		PHALCON_CALL_SELF(&style, "getstyle", type);

		PHALCON_INIT_NVAR(replace_pairs);
		array_init(replace_pairs);

		phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
		phalcon_array_update_str(replace_pairs, SL(":var"), variable, PH_COPY);

		PHALCON_CALL_FUNCTION(&output, "strtr", str, replace_pairs);

		PHALCON_SCONCAT(return_value, output);
	}

	RETURN_MM();
}

/**
 * Returns an HTML string of information about a single variable.
 *
 * <code>
 *    echo (new \Phalcon\Debug\Dump())->variable($foo, "foo");
 * </code>
 */
PHP_METHOD(Phalcon_Debug_Dump, variable){

	zval *variable, *name = NULL, *str, *type, *style = NULL, *output = NULL, *replace_pairs;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &variable, &name);

	if (!name) {
		name = &PHALCON_GLOBAL(z_null);
	}

	PHALCON_INIT_VAR(str);
	ZVAL_STRING(str, "<pre style=':style'>:output</pre>");

	PHALCON_INIT_VAR(type);
	ZVAL_STRING(type, "pre");

	PHALCON_CALL_SELF(&style, "getstyle", type);
	PHALCON_CALL_SELF(&output, "output", variable, name);

	PHALCON_INIT_VAR(replace_pairs);
	array_init(replace_pairs);

	phalcon_array_update_str(replace_pairs, SL(":style"), style, PH_COPY);
	phalcon_array_update_str(replace_pairs, SL(":output"), output, PH_COPY);

	PHALCON_RETURN_CALL_FUNCTION("strtr", str, replace_pairs);

	RETURN_MM();
}

/**
 * Returns an HTML string of debugging information about any number of
 * variables, each wrapped in a "pre" tag.
 *
 * <code>
 *    $foo = "string";
 *    $bar = ["key" => "value"];
 *    $baz = new stdClass();
 *    echo (new \Phalcon\Debug\Dump())->variables($foo, $bar, $baz);
 *</code>
 *
 * @param mixed variable
 * @param ...
 */
PHP_METHOD(Phalcon_Debug_Dump, variables){

	zval *arg_list = NULL, *variable = NULL, *output = NULL;

	PHALCON_MM_GROW();

	PHALCON_CALL_FUNCTION(&arg_list, "func_get_args");

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(arg_list), variable) {
		PHALCON_CALL_SELF(&output, "variable", variable);
		PHALCON_SCONCAT(return_value, output);
	} ZEND_HASH_FOREACH_END();

	RETURN_MM();
}
