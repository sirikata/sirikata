/*  Sirikata - Mono Embedding
 *  MonoUtil.cpp
 *
 *  Copyright (c) 2009, Stanford University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sirikata/oh/Platform.hpp>
#include "MonoUtil.hpp"
#include "MonoException.hpp"

#include <stdlib.h>
#include <cstring>

#include <mono/metadata/metadata.h>
#include <mono/utils/mono-digest.h>

#include <glib.h>
#include <mono/jit/jit.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/profiler.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/attrdefs.h>

#include "MonoClass.hpp"
#include "MonoObject.hpp"
#include "MonoMethodLookupCache.hpp"

namespace Mono {


extern "C" {

#if NO_UNALIGNED_ACCESS

extern guint16 mono_read16 (const unsigned char *x);
extern guint32 mono_read32 (const unsigned char *x);
extern guint64 mono_read64 (const unsigned char *x);

#define read16(x) (mono_read16 ((const unsigned char *)(x)))
#define read32(x) (mono_read32 ((const unsigned char *)(x)))
#define read64(x) (mono_read64 ((const unsigned char *)(x)))

#else

#define read16(x) GUINT16_FROM_LE (*((const guint16 *) (x)))
#define read32(x) GUINT32_FROM_LE (*((const guint32 *) (x)))
#define read64(x) GUINT64_FROM_LE (*((const guint64 *) (x)))

#endif

}

static gchar*
encode_public_tok (const guchar *token, gint32 len)
{
	const static gchar allowed [] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	gchar *res;
	int i;

	res = (gchar*)g_malloc (len * 2 + 1);
	for (i = 0; i < len; i++) {
		res [i * 2] = allowed [token [i] >> 4];
		res [i * 2 + 1] = allowed [token [i] & 0xF];
	}
	res [len * 2] = 0;
	return res;
}

/*
 * mono_assembly_name_free:
 * @param aname assembly name to free
 *
 */
void
mono_assembly_name_free (MonoAssemblyName *aname)
{
	if (aname == NULL)
		return;

	g_free ((void *) aname->name);
	g_free ((void *) aname->culture);
	g_free ((void *) aname->hash_value);
}

static gboolean
parse_public_key (const gchar *key, gchar** pubkey)
{
	const gchar *pkey;
	gchar header [16], val, *arr;
	gint i, j, offset, bitlen, keylen, pkeylen;

	keylen = strlen (key) >> 1;
	if (keylen < 1)
		return FALSE;

	val = g_ascii_xdigit_value (key [0]) << 4;
	val |= g_ascii_xdigit_value (key [1]);
	switch (val) {
		case 0x00:
			if (keylen < 13)
				return FALSE;
			val = g_ascii_xdigit_value (key [24]);
			val |= g_ascii_xdigit_value (key [25]);
			if (val != 0x06)
				return FALSE;
			pkey = key + 24;
			break;
		case 0x06:
			pkey = key;
			break;
		default:
			return FALSE;
	}

	/* We need the first 16 bytes
	* to check whether this key is valid or not */
	pkeylen = strlen (pkey) >> 1;
	if (pkeylen < 16)
		return FALSE;

	for (i = 0, j = 0; i < 16; i++) {
		header [i] = g_ascii_xdigit_value (pkey [j++]) << 4;
		header [i] |= g_ascii_xdigit_value (pkey [j++]);
	}

	if (header [0] != 0x06 || /* PUBLICKEYBLOB (0x06) */
			header [1] != 0x02 || /* Version (0x02) */
			header [2] != 0x00 || /* Reserved (word) */
			header [3] != 0x00 ||
			(guint)(read32 (header + 8)) != 0x31415352) /* DWORD magic = RSA1 */
		return FALSE;

	/* Based on this length, we _should_ be able to know if the length is right */
	bitlen = read32 (header + 12) >> 3;
	if ((bitlen + 16 + 4) != pkeylen)
		return FALSE;

	/* Encode the size of the blob */
	offset = 0;
	if (keylen <= 127) {
            arr = (gchar*) g_malloc (keylen + 1);
		arr [offset++] = keylen;
	} else {
            arr = (gchar*)g_malloc (keylen + 2);
            arr [offset++] = (char)(unsigned char)0x80; /* 10bs */
		arr [offset++] = keylen;
	}

	for (i = offset, j = 0; i < keylen + offset; i++) {
		arr [i] = g_ascii_xdigit_value (key [j++]) << 4;
		arr [i] |= g_ascii_xdigit_value (key [j++]);
	}
	if (pubkey)
		*pubkey = arr;

	return TRUE;
}

static gboolean
build_assembly_name (const char *name, const char *version, const char *culture, const char *token, const char *key, MonoAssemblyName *aname, gboolean save_public_key)
{
	gint major, minor, build, revision;
	gint len;
	gint version_parts;
	gchar *pkey, *pkeyptr, *encoded, tok [8];

	memset (aname, 0, sizeof (MonoAssemblyName));

	if (version) {
		version_parts = sscanf (version, "%u.%u.%u.%u", &major, &minor, &build, &revision);
		if (version_parts < 2 || version_parts > 4)
			return FALSE;

		/* FIXME: we should set build & revision to -1 (instead of 0)
		if these are not set in the version string. That way, later on,
		we can still determine if these were specified.	*/
		aname->major = major;
		aname->minor = minor;
		if (version_parts >= 3)
			aname->build = build;
		else
			aname->build = 0;
		if (version_parts == 4)
			aname->revision = revision;
		else
			aname->revision = 0;
	}

	aname->name = g_strdup (name);

	if (culture) {
		if (g_ascii_strcasecmp (culture, "neutral") == 0)
			aname->culture = g_strdup ("");
		else
			aname->culture = g_strdup (culture);
	}

	if (token && strncmp (token, "null", 4) != 0) {
		char *lower = g_ascii_strdown (token, MONO_PUBLIC_KEY_TOKEN_LENGTH);
		g_strlcpy ((char*)aname->public_key_token, lower, MONO_PUBLIC_KEY_TOKEN_LENGTH);
		g_free (lower);
	}

	if (key && strncmp (key, "null", 4) != 0) {
		if (!parse_public_key (key, &pkey)) {
			mono_assembly_name_free (aname);
			return FALSE;
		}

		len = mono_metadata_decode_blob_size ((const gchar *) pkey, (const gchar **) &pkeyptr);
		// We also need to generate the key token
		mono_digest_get_public_token ((guchar*) tok, (guint8*) pkeyptr, len);
		encoded = encode_public_tok ((guchar*) tok, 8);
		g_strlcpy ((gchar*)aname->public_key_token, encoded, MONO_PUBLIC_KEY_TOKEN_LENGTH);
		g_free (encoded);

		if (save_public_key)
			aname->public_key = (guint8*) pkey;
		else
			g_free (pkey);
	}

	return TRUE;
}

static gboolean
parse_assembly_directory_name (const char *name, const char *dirname, MonoAssemblyName *aname)
{
	gchar **parts;
	gboolean res;

	parts = g_strsplit (dirname, "_", 3);
	if (!parts || !parts[0] || !parts[1] || !parts[2]) {
		g_strfreev (parts);
		return FALSE;
	}

	res = build_assembly_name (name, parts[0], parts[1], parts[2], NULL, aname, FALSE);
	g_strfreev (parts);
	return res;
}

gboolean
mono_assembly_name_parse_full (const char *name, MonoAssemblyName *aname, gboolean save_public_key, gboolean *is_version_defined)
{
	gchar *dllname;
	gchar *version = NULL;
	gchar *culture = NULL;
	gchar *token = NULL;
	gchar *key = NULL;
	gboolean res;
	gchar *value;
	gchar **parts;
	gchar **tmp;
	gboolean version_defined;

	if (!is_version_defined)
		is_version_defined = &version_defined;
	*is_version_defined = FALSE;

	parts = tmp = g_strsplit (name, ",", 4);
	if (!tmp || !*tmp) {
		g_strfreev (tmp);
		return FALSE;
	}

	dllname = g_strstrip (*tmp);

	tmp++;

	while (*tmp) {
		value = g_strstrip (*tmp);
		if (!g_ascii_strncasecmp (value, "Version=", 8)) {
			*is_version_defined = TRUE;
			version = g_strstrip (value + 8);
			tmp++;
			continue;
		}

		if (!g_ascii_strncasecmp (value, "Culture=", 8)) {
			culture = g_strstrip (value + 8);
			tmp++;
			continue;
		}

		if (!g_ascii_strncasecmp (value, "PublicKeyToken=", 15)) {
			token = g_strstrip (value + 15);
			tmp++;
			continue;
		}

		if (!g_ascii_strncasecmp (value, "PublicKey=", 10)) {
			key = g_strstrip (value + 10);
			tmp++;
			continue;
		}

		g_strfreev (parts);
		return FALSE;
	}

	res = build_assembly_name (dllname, version, culture, token, key, aname, save_public_key);
	g_strfreev (parts);
	return res;
}

/**
 * mono_assembly_name_parse:
 * @param name name to parse
 * @param aname the destination assembly name
 *
 * Parses an assembly qualified type name and assigns the name,
 * version, culture and token to the provided assembly name object.
 *
 * @returns true if the name could be parsed.
 */
gboolean
mono_assembly_name_parse (const char *name, MonoAssemblyName *aname)
{
	return mono_assembly_name_parse_full (name, aname, FALSE, NULL);
}





void checkNullReference(MonoObject* obj) {
    if (obj == NULL)
        throw Exception::NullReference();
}



#define MAX_ARGUMENTS 32
#define MAX_MATCHES 32

enum MatchComparison {
    WORSE_MATCH,
    EQUAL_MATCH,
    BETTER_MATCH
};

//#####################################################################
// Function compareMatches
//#####################################################################

// Compares the (possible) conversions of best_match_param and check_param
// and determines which one is better or if they're equal.
//
// FIXME this should really follow all the rules.  Currently known incorrect
// behavior: doesn't handle cast operators (only casts via the class
// hierarchy) and doesn't handle all the integer cast preferences listed in
// the spec (http://www.go-mono.org/docs/index.aspx?link=ecmaspec%3A14.4.2)
static MatchComparison compareMatches(MonoClass* input_arg, MonoClass* best_match_param, MonoClass* check_param) {
    // fast path - if both are the same they're equally good matches
    if (check_param == best_match_param)
        return EQUAL_MATCH;

    // if it matches and there was no previous match then its better
    if (best_match_param == NULL)
        return BETTER_MATCH;

    // if the new type is a subclass of the previous best, then it is a
    // closer match
    if (mono_class_is_subclass_of(check_param, best_match_param, TRUE) == TRUE)
        return BETTER_MATCH;
    // and vice versa
    if (mono_class_is_subclass_of(best_match_param, check_param, TRUE) == TRUE)
        return WORSE_MATCH;

    // otherwise neither one is better
    return EQUAL_MATCH;
}

//#####################################################################
// Function lookupMethod
//#####################################################################
static MonoMethod* lookupMethod(MonoClass* klass, MonoObject* this_ptr, const char* name, MonoObject* args[], guint32 nargs) {
    assert(this_ptr == NULL || klass == mono_object_get_class(this_ptr));

    MonoMethod* best_matches[MAX_MATCHES];
    memset(best_matches, 0, MAX_MATCHES*sizeof(MonoMethod*));
    int nmatches = 0;

//    printf("****\n");

    // Iterate to find the set of methods that have matching signatures
    for(MonoClass* k = klass; k; k = mono_class_get_parent(k)) {
        gpointer method_iter = NULL;
        while( MonoMethod* method = mono_class_get_methods(k, &method_iter) ) {
            const char* method_name = mono_method_get_name(method);
            MonoMethodSignature* check_signature = mono_method_signature(method);
            guint32 nparams = mono_signature_get_param_count(check_signature);

//            printf("%s(%d) %d %d", method_name, mono_signature_get_call_conv(check_signature), nparams, mono_signature_vararg_start(check_signature));

            if ( strcmp(method_name, name) != 0 ||
                nparams != nargs ) {
//                printf("\n");
                continue;
            }

            bool match = true;
            int arg_idx = 0;
            gpointer check_param_iter = NULL;

//            printf(" (");
            while( MonoType* check_param_type = mono_signature_get_params(check_signature, &check_param_iter) ) {
                MonoClass* check_param_class = mono_class_from_mono_type(check_param_type);
                if (args[arg_idx] != NULL) {
                    MonoClass* input_arg_class = mono_object_get_class(args[arg_idx]);

//                printf("%s,", mono_class_get_name(check_param_class));

                    if (!mono_class_is_subclass_of(input_arg_class, check_param_class, TRUE))
                        match = false;
                }
                arg_idx++;
            }

//            printf(")\n");

            if (match) {
                best_matches[nmatches] = method;
                nmatches++;
            }
        }
    }


    // With the list of all matches, try to find one that's better than all the rest
    // FIXME this is horribly inefficient, but shouldn't be that bad since there are usually
    // only a few overloads at most.  This should be replaced by a better mechanism at
    // some point, i.e. something a compiler would use
    for(int bidx = 0; bidx < nmatches; bidx++) {
        bool worse_than_any = false;

        for(int cidx = 0; cidx < nmatches; cidx++) {
            if (bidx == cidx) continue;

            MonoMethodSignature* bsignature = mono_method_signature(best_matches[bidx]);
            MonoMethodSignature* csignature = mono_method_signature(best_matches[cidx]);

            gpointer b_param_iter = NULL;
            gpointer c_param_iter = NULL;
            MonoType* b_param_type;
            MonoType* c_param_type;
            int arg_idx = 0;
            bool any_better_params = false;
            bool any_worse_params = false;

            while( (b_param_type = mono_signature_get_params(bsignature, &b_param_iter)) &&
                (c_param_type = mono_signature_get_params(csignature, &c_param_iter) ) ) {
                MonoClass* b_param_class = mono_class_from_mono_type(b_param_type);
                MonoClass* c_param_class = mono_class_from_mono_type(c_param_type);

                if (args[arg_idx] != NULL) {
                    MonoClass* input_arg_class = mono_object_get_class(args[arg_idx]);

                    MatchComparison compare = compareMatches(input_arg_class, b_param_class, c_param_class);
                    if ( compare == WORSE_MATCH )
                        any_worse_params = true;
                    if ( compare == BETTER_MATCH )
                        any_better_params = true;
                }
                arg_idx++;
            }

            if (any_worse_params ||
                (!any_better_params &&
                    mono_class_is_subclass_of(
                        mono_method_get_class(best_matches[cidx]),
                        mono_method_get_class(best_matches[bidx]),
                        TRUE
                    )
                )
            ) {
                worse_than_any = true;
                break;
            }
        }

        if (!worse_than_any) {
            return best_matches[bidx];
        }
    }

    throw Exception::MissingMethod(Class(klass), std::string(name));

    return NULL;
}

void* object_to_raw(MonoClass* receive_klass, MonoObject* obj) {
    if ( mono_class_is_valuetype( receive_klass ) ) {
        return mono_object_unbox(obj);
    } else {
        return obj;
    }
}

//#####################################################################
// Function send_method
//#####################################################################
Object send_method(MonoObject* this_ptr, MonoMethod* method, MonoObject* args[], int nargs) {
    void* mixed_args[MAX_ARGUMENTS];

    gpointer sig_param_iter = NULL;
    MonoType* sig_param_type;
    MonoMethodSignature* signature = mono_method_signature(method);
    int param_idx = 0;
    while( (sig_param_type = mono_signature_get_params(signature, &sig_param_iter)) ) {
        mixed_args[param_idx] = object_to_raw( mono_class_from_mono_type(sig_param_type), args[param_idx] );
        param_idx++;
    }
    mixed_args[nargs] = NULL;

    MonoObject* exception = NULL;
    void* mono_this_ptr = this_ptr;
    if (this_ptr != NULL && mono_class_is_valuetype (mono_method_get_class (method)))
        mono_this_ptr = mono_object_unbox(this_ptr);
    MonoObject* retval = mono_runtime_invoke(method, mono_this_ptr, mixed_args, &exception);

    if (exception != NULL) {
        Object exception_obj(exception);
        Exception exc(exception_obj);
        std::cout << exc << std::endl;
        throw exc;
    }

    return Object(retval);
}

//#####################################################################
// Function send_base
//#####################################################################
Object send_base(MonoObject* this_ptr, const char* message, MonoObject* args[], int nargs, MethodLookupCache* cache) {
    assert(this_ptr != NULL);
    MonoClass* this_class = mono_object_get_class(this_ptr);
    MonoMethod* method = NULL;
    if (cache != NULL)
        method = cache->lookup(this_class, message, args, nargs);
    if (method == NULL) {
        method = lookupMethod(this_class, this_ptr, message, args, nargs);
        assert(method != NULL);
        if (cache != NULL)
            cache->update(this_class, message, args, nargs, method);
    }

    return send_method(this_ptr, method, args, nargs);
}

//#####################################################################
// Function send_base
//#####################################################################
Object send_base(MonoClass* klass, const char* message, MonoObject* args[], int nargs, MethodLookupCache* cache) {
    MonoMethod* method = NULL;
    if (cache != NULL)
        method = cache->lookup(klass, message, args, nargs);
    if (method == NULL) {
        method = lookupMethod(klass, NULL, message, args, nargs);
        assert(method != NULL);
        if (cache != NULL)
            cache->update(klass, message, args, nargs, method);
    }

    return send_method(NULL, method, args, nargs);
}


//#####################################################################
// Function mono_object_get_property
//#####################################################################
static MonoProperty* mono_object_get_property(MonoObject* obj, const char* name) {
    MonoClass* klass = mono_object_get_class(obj);
    assert(klass != NULL);

    MonoProperty* prop = mono_class_get_property_from_name( klass, name );
    if (prop == NULL)
        throw Exception::MissingProperty(Class(klass), name);

    return prop;
}

//#####################################################################
// Function mono_object_get_property_get
//#####################################################################
MonoMethod* mono_object_get_property_get(MonoObject* obj, const char* name) {
    MonoProperty* prop = mono_object_get_property(obj, name);

    MonoMethod* prop_get_method = mono_property_get_get_method(prop);
    if (prop_get_method == NULL)
        throw Exception::MissingMethod(Class(mono_object_get_class(obj)), name);

    return prop_get_method;
}

//#####################################################################
// Function mono_object_get_property_set
//#####################################################################
MonoMethod* mono_object_get_property_set(MonoObject* obj, const char* name) {
    MonoProperty* prop = mono_object_get_property(obj, name);

    MonoMethod* prop_set_method = mono_property_get_set_method(prop);
    if (prop_set_method == NULL)
        throw Exception::MissingMethod(Class(mono_object_get_class(obj)), name);

    return prop_set_method;
}


//#####################################################################
// Function NativePtrToUInt64
//#####################################################################
SIRIKATA_PLUGIN_EXPORT_C
Sirikata::uint64 NativePtrToUInt64(void* ptr) {
    return (Sirikata::uint64)ptr;
}

//#####################################################################
// Function FreeNativeString
//#####################################################################
SIRIKATA_PLUGIN_EXPORT_C
void FreeNativeString(void* ptr) {
    delete[] ((char*)ptr);
}



static int memory_usage_array(MonoArray *array, GHashTable *visited);
static int memory_usage(MonoObject *obj, GHashTable *visited);

static int memory_usage_array(MonoArray *array, GHashTable *visited) {
    int total = 0;
    MonoClass *array_class = mono_object_get_class ((MonoObject *) array);
    MonoClass *element_class = mono_class_get_element_class (array_class);
    MonoType *element_type = mono_class_get_type (element_class);

    if (MONO_TYPE_IS_REFERENCE (element_type)) {
        for (guint32 i = 0; i < mono_array_length (array); i++) {
            MonoObject *element = (MonoObject*) mono_array_get (array, gpointer, i);

            if (element != NULL)
                total += memory_usage (element, visited);
        }
    }

    return total;
}

static int memory_usage(MonoObject *obj, GHashTable *visited) {
    static int indent = 0;

    int total = 0;
    MonoClass *klass;
    MonoType *type;

    if (g_hash_table_lookup (visited, obj))
        return 0;

    g_hash_table_insert (visited, obj, obj);

    klass = mono_object_get_class (obj);
    type = mono_class_get_type (klass);

    /* This is an array, so drill down into it */
    if (type->type == MONO_TYPE_SZARRAY)
        total += memory_usage_array ((MonoArray *) obj, visited);

    while(klass) {
        gpointer iter = NULL;
        MonoClassField *field;
        while ((field = mono_class_get_fields (klass, &iter)) != NULL) {
            MonoType *ftype = mono_field_get_type (field);
            gpointer value;

//            for(int i = 0; i < indent; i++)
//                printf(".");
//            printf("%s   ", mono_field_get_name(field));

            if ((ftype->attrs & (MONO_FIELD_ATTR_STATIC | MONO_FIELD_ATTR_HAS_RVA)) != 0)
                continue;

//        for(int i = 0; i < indent; i++)
//            printf(".");
//        printf ("Field type: 0x%x\n", ftype->type);

            /* FIXME: There are probably other types we need to drill down into */
            switch (ftype->type) {
              case MONO_TYPE_CLASS:
              case MONO_TYPE_OBJECT:
                mono_field_get_value (obj, field, &value);

                if (value != NULL) {
                    indent++;
                    total += memory_usage ((MonoObject *) value, visited);
                indent--;
                }
                break;

              case MONO_TYPE_STRING:
                mono_field_get_value (obj, field, &value);
                if (value != NULL)
                    total += mono_object_get_size ((MonoObject *) value);
                break;

              case MONO_TYPE_SZARRAY:
                mono_field_get_value (obj, field, &value);

                if (value != NULL) {
                    total += memory_usage_array ((MonoArray *) value, visited);
                    total += mono_object_get_size ((MonoObject *) value);
                }

                break;

              case MONO_TYPE_BOOLEAN:
              case MONO_TYPE_CHAR:
              case MONO_TYPE_I1:
              case MONO_TYPE_U1:
              case MONO_TYPE_I2:
              case MONO_TYPE_U2:
              case MONO_TYPE_I4:
              case MONO_TYPE_U4:
              case MONO_TYPE_I8:
              case MONO_TYPE_U8:
              case MONO_TYPE_R4:
              case MONO_TYPE_R8:
              case MONO_TYPE_ENUM:
              case MONO_TYPE_I:
              case MONO_TYPE_U:
              case MONO_TYPE_VALUETYPE:
                /* ignore, this will be included in mono_object_get_size() */
                break;

              default:
                printf ("Got type 0x%x for %s %s\n", ftype->type, mono_class_get_name(klass), mono_field_get_name(field));
                break;
            }
        }

        klass = mono_class_get_parent(klass);
    }

    total += mono_object_get_size (obj);

    //printf("size of %s: %d\n", mono_class_get_name(mono_object_get_class (obj)), mono_object_get_size(obj));

    return total;
}

int memory_usage_from_object_root(MonoObject* obj)
{
    GHashTable *visited = g_hash_table_new (NULL, NULL);
    int n;

    n = memory_usage (obj, visited);

    g_hash_table_destroy (visited);

    return n;
}


} // namespace Mono
