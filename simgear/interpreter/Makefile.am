includedir = @includedir@/js

lib_LIBRARIES = libsginterp.a

include_HEADERS = \
	interpreter.hxx

libsginterp_a_SOURCES = \
	interpreter.cxx interpreter.hxx \
        ixlib_javascript.hh \
	js_array.cc \
	js_expression.cc \
	js_interpreter.cc  \
	js_value.cc \
	js_declaration.cc \
	js_instruction.cc  \
	js_library.cc \
	lex.javascript.cc ixlib_token_lex.hh ixlib_token_javascript.hh \
	scanner.cc ixlib_scanner.hh ixlib_scanjs.hh \
	exbase.cc \
	numeric.cc ixlib_numeric.hh \
	numconv.cc ixlib_numconv.hh \
	re.cc ixlib_re.hh ixlib_re_impl.hh \
	exgen.cc ixlib_exgen.hh \
	string.cc ixlib_string.hh \
	ixlib_base.hh \
	ixlib_exbase.hh \
	ixlib_garbage.hh \
	ixlib_i18n.hh \
	ixlib_js_internals.hh \
	ixlib_random.hh

INCLUDES = -I$(top_srcdir)
