/******************************************************************************
 * Copyright (c) 2013, GeoData Institute (www.geodata.soton.ac.uk)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/**
 * @file error.cpp
 * @brief This defines the `MapserverError` class.
 */

#include "error.hpp"

Persistent<String> MapserverError::MapserverError_symbol;

/**
 * @defgroup error_properties Properties of the error object
 *
 * These represent property names that are added to javascript `Error` objects
 * generated by mapserver.
 *
 * @{
 */
Persistent<String> MapserverError::name_symbol;
Persistent<String> MapserverError::code_symbol;
Persistent<String> MapserverError::category_symbol;
Persistent<String> MapserverError::routine_symbol;
Persistent<String> MapserverError::isReported_symbol;
Persistent<String> MapserverError::errorStack_symbol;
/**@}*/


/**
 * @details This is called from the module initialisation function
 * when the module is first loaded by Node. It should only be called
 * once per process.
 */
void MapserverError::Init() {
  // initialise the persistent strings
  MapserverError_symbol = NODE_PSYMBOL("MapserverError");

  name_symbol = NODE_PSYMBOL("name");
  code_symbol = NODE_PSYMBOL("code");
  category_symbol = NODE_PSYMBOL("category");
  routine_symbol = NODE_PSYMBOL("routine");
  isReported_symbol = NODE_PSYMBOL("isReported");
  errorStack_symbol = NODE_PSYMBOL("errorStack");
}

/**
 * @details A constructor that builds from a standard mapserver `errorObj`.
 * This effectively copies the mapserver data structure.
 *
 * @param error A mapserver `errorObj`.
 */
MapserverError::MapserverError(const errorObj *error) {
  MapserverError *copy = this;
  length = 0;
  while (error) {
    copy->code = error->code;
    copy->routine = error->routine;
    copy->message = error->message;
    copy->isReported = error->isreported;
    if (error->next) {
      copy->next = new MapserverError();
      copy = copy->next;
    }
    error = error->next;
    length++;
  }
  copy->next = NULL;
}

/**
 * @details This returns a representation of the `MapserverError` instance as a
 * V8 exception.  The internal linked list implementing the error stack is
 * converted to a Javascript Array.
 */
Handle<Value> MapserverError::toV8Error() {
  HandleScope scope;

  // Represent the error stack linked list as an array
  MapserverError *error = next;
  unsigned int i = 0;
  Local<Array> errorStack = Array::New(length-1);
  while (error) {
    Handle<Value> exception = ToV8Error(error);
    errorStack->Set(i++, exception);   // add the error to the stack
    error = error->next;
  }

  // Create an error representing the current error
  Handle<Value> result = ToV8Error(this);
  result->ToObject()->Set(errorStack_symbol, errorStack); // add the stack to the current error

  return scope.Close(result);
}

/**
 * @detail A class method that converts a `MapserverError` to a V8 exception.
 * This only operates on the error properties and does not process the internal
 * linked list.
 *
 * @param error The `MapserverError` pointer.
 */
Handle<Value> MapserverError::ToV8Error(MapserverError *error) {
  HandleScope scope;

  char *category = msGetErrorCodeString(error->code);
  Local<Value> result = Exception::Error(String::New(( error->message.length() ? error->message.c_str() : category )));
  Local<Object> object = result->ToObject();
  object->Set(name_symbol, MapserverError_symbol);
  object->Set(routine_symbol, String::New(error->routine.c_str()));
  object->Set(code_symbol, Integer::New(error->code));
  object->Set(category_symbol, String::New(category));
  object->Set(isReported_symbol, Boolean::New(error->isReported));

  return scope.Close(result);
}
