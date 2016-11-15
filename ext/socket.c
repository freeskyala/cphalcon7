
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
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

#include "socket.h"
#include "socket/client.h"
#include "socket/exception.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/fcall.h"
#include "kernel/string.h"
#include "kernel/array.h"
#include "kernel/operators.h"
#include "kernel/concat.h"
#include "kernel/object.h"
#include "kernel/exception.h"


/**
 * Phalcon\Socket
 *
 * A simple wrapper for PHP's socket functions
 */
zend_class_entry *phalcon_socket_ce;

PHP_METHOD(Phalcon_Socket, getSocket);
PHP_METHOD(Phalcon_Socket, getSocketId);
PHP_METHOD(Phalcon_Socket, _throwSocketException);
PHP_METHOD(Phalcon_Socket, setBlocking);
PHP_METHOD(Phalcon_Socket, isBlocking);
PHP_METHOD(Phalcon_Socket, setOption);
PHP_METHOD(Phalcon_Socket, close);
PHP_METHOD(Phalcon_Socket, __destruct);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_socket_setblocking, 0, 0, 1)
	ZEND_ARG_INFO(0, flag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_socket_setoption, 0, 0, 3)
	ZEND_ARG_TYPE_INFO(0, level, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, optname, IS_LONG, 0)
	ZEND_ARG_INFO(0, optval)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_socket_method_entry[] = {
	PHP_ME(Phalcon_Socket, getSocket, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Socket, getSocketId, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Socket, _throwSocketException, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Socket, setBlocking, arginfo_phalcon_socket_setblocking, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Socket, isBlocking, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Socket, setOption, arginfo_phalcon_socket_setoption, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Socket, close, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Socket, __destruct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_FE_END
};

/**
 * Phalcon\Socket initializer
 */
PHALCON_INIT_CLASS(Phalcon_Socket){

	PHALCON_REGISTER_CLASS(Phalcon, Socket, socket, phalcon_socket_method_entry, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);

	zend_declare_property_null(phalcon_socket_ce, SL("_socket"),	ZEND_ACC_PROTECTED);
	zend_declare_property_long(phalcon_socket_ce, SL("_domain"),	PHALCON_SOCKET_AF_INET, ZEND_ACC_PROTECTED);
	zend_declare_property_long(phalcon_socket_ce, SL("_type"),		PHALCON_SOCKET_SOCK_STREAM, ZEND_ACC_PROTECTED);
	zend_declare_property_long(phalcon_socket_ce, SL("_protocol"),	PHALCON_SOCKET_SOL_TCP, ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_socket_ce, SL("_blocking"),	1, ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_socket_ce, SL("_address"),	ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_socket_ce, SL("_port"),		ZEND_ACC_PROTECTED);

	zend_declare_class_constant_long(phalcon_socket_ce, SL("AF_UNIX"),	PHALCON_SOCKET_AF_UNIX);
	zend_declare_class_constant_long(phalcon_socket_ce, SL("AF_INET"),	PHALCON_SOCKET_AF_INET);
#if HAVE_IPV6
	zend_declare_class_constant_long(phalcon_socket_ce, SL("AF_INET6"),	PHALCON_SOCKET_AF_INET6);
#endif
	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOCK_STREAM"),		PHALCON_SOCKET_SOCK_STREAM);
	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOCK_DGRAM"),		PHALCON_SOCKET_SOCK_DGRAM);
	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOCK_RAW"),			PHALCON_SOCKET_SOCK_RAW);
	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOCK_SEQPACKET"),	PHALCON_SOCKET_SOCK_SEQPACKET);
	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOCK_RDM"),			PHALCON_SOCKET_SOCK_RDM);

	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOL_TCP"),			PHALCON_SOCKET_SOL_TCP);
	zend_declare_class_constant_long(phalcon_socket_ce, SL("SOL_UDP"),			PHALCON_SOCKET_SOL_UDP);
	return SUCCESS;
}

/**
 * Gets the socket
 *
 * @return resource
 */
PHP_METHOD(Phalcon_Socket, getSocket){

	RETURN_MEMBER(getThis(), "_socket");
}

/**
 * Gets the socket id
 *
 * @return int
 */
PHP_METHOD(Phalcon_Socket, getSocketId){

	zval socket = {};

	phalcon_read_property(&socket, getThis(), SL("_socket"), PH_NOISY);
	ZVAL_LONG(return_value, Z_RES_HANDLE(socket));
}

/**
 * Throws an socket exception
 */
PHP_METHOD(Phalcon_Socket, _throwSocketException){

	zval socket = {}, exception_code = {}, exception_message = {}, exception = {};

	phalcon_read_property(&socket, getThis(), SL("_socket"), PH_NOISY);
	if (Z_TYPE(socket) == IS_RESOURCE) {
		PHALCON_CALL_FUNCTIONW(&exception_code, "socket_last_error", &socket);
	} else {
		PHALCON_CALL_FUNCTIONW(&exception_code, "socket_last_error");
	}
	PHALCON_CALL_FUNCTIONW(&exception_message, "socket_strerror", &exception_code);

	if (Z_TYPE(socket) == IS_RESOURCE) {
		PHALCON_CALL_FUNCTIONW(NULL, "socket_clear_error", &socket);
	} else {
		PHALCON_CALL_FUNCTIONW(NULL, "socket_clear_error");
	}

	object_init_ex(&exception, phalcon_socket_exception_ce);
	PHALCON_CALL_METHODW(NULL, &exception, "__construct", &exception_message, &exception_code);

	phalcon_throw_exception(&exception);
}

/**
 * Set the socket to blocking / non blocking
 *
 * @param int $flag
 * @return boolean
 */
PHP_METHOD(Phalcon_Socket, setBlocking){

	zval *flag, socket = {};

	phalcon_fetch_params(0, 1, 0, &flag);

	phalcon_read_property(&socket, getThis(), SL("_socket"), PH_NOISY);

	if (zend_is_true(flag)) {
		PHALCON_CALL_FUNCTIONW(return_value, "socket_set_block", &socket);
	} else {
		PHALCON_CALL_FUNCTIONW(return_value, "socket_set_nonblock", &socket);
	}
	phalcon_update_property_zval(getThis(), SL("_blocking"), flag);
}

/**
 * Checks the socket blocking / non blocking
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Socket, isBlocking){

	RETURN_MEMBER(getThis(), "_blocking");
}

/**
 * Set the socket to blocking / non blocking
 *
 * @param int $level
 * @param int $optname
 * @param mixed $optval 
 * @return boolean
 */
PHP_METHOD(Phalcon_Socket, setOption){

	zval *level, *optname, *optval, socket = {};

	phalcon_fetch_params(0, 3, 0, &level, &optname, &optval);

	phalcon_read_property(&socket, getThis(), SL("_socket"), PH_NOISY);

	PHALCON_CALL_FUNCTIONW(return_value, "socket_set_option", &socket, level, optname, optval);
	
	if (PHALCON_IS_FALSE(return_value)) {
		PHALCON_CALL_METHODW(NULL, getThis(), "_throwsocketexception");
	}
}

/**
 * Close the socket
 */
PHP_METHOD(Phalcon_Socket, close){

	zval socket = {};

	phalcon_read_property(&socket, getThis(), SL("_socket"), PH_NOISY);

	PHALCON_CALL_FUNCTIONW(NULL, "socket_close", &socket);
}

/**
 * Cleans up the socket and the resource.
 */
PHP_METHOD(Phalcon_Socket, __destruct){

	zval socket = {};

	phalcon_read_property(&socket, getThis(), SL("_socket"), PH_NOISY);
	PHALCON_CALL_METHODW(NULL, getThis(), "close");
	phalcon_update_property_null(getThis(), SL("_socket"));
}