/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsCRTGlue_h__
#define nsCRTGlue_h__

#include "nscore.h"

/**
 * Scan a string for the first character that is *not* in a set of
 * delimiters.  If the string is only delimiter characters, the end of the
 * string is returned.
 *
 * @param delims The set of delimiters (null-terminated)
 * @param str    The string to search (null-terminated)
 */
NS_COM_GLUE const char*
NS_strspnp(const char *delims, const char *str);

/**
 * Tokenize a string. This function is similar to the strtok function in the
 * C standard library, but it does not use static variables to maintain state
 * and is therefore thread and reentrancy-safe.
 *
 * Any leading delimiters in str are skipped. Then the string is scanned
 * until an additional delimiter or end-of-string is found. The final
 * delimiter is set to '\0'.
 *
 * @param delims The set of delimiters.
 * @param str    The string to search. This is an in-out parameter; it is
 *               reset to the end of the found token + 1, or to the
 *               end-of-string if there are no more tokens.
 * @return       The token. If no token is found (the string is only
 *               delimiter characters), NULL is returned.
 */
NS_COM_GLUE char*
NS_strtok(const char *delims, char **str);

/**
 * "strlen" for PRUnichar strings
 */
NS_COM_GLUE uint32_t
NS_strlen(const PRUnichar *aString);

/**
 * "strcmp" for PRUnichar strings
 */
NS_COM_GLUE int
NS_strcmp(const PRUnichar *a, const PRUnichar *b);

/**
 * "strdup" for PRUnichar strings, uses the NS_Alloc allocator.
 */
NS_COM_GLUE PRUnichar*
NS_strdup(const PRUnichar *aString);

/**
 * "strdup", but using the NS_Alloc allocator.
 */
NS_COM_GLUE char*
NS_strdup(const char *aString);

/**
 * strndup for PRUnichar strings... this function will ensure that the
 * new string is null-terminated. Uses the NS_Alloc allocator.
 */
NS_COM_GLUE PRUnichar*
NS_strndup(const PRUnichar *aString, uint32_t aLen);

// The following case-conversion methods only deal in the ascii repertoire
// A-Z and a-z

// semi-private data declarations... don't use these directly.
class NS_COM_GLUE nsLowerUpperUtils {
public:
  static const unsigned char kLower2Upper[256];
  static const unsigned char kUpper2Lower[256];
};

inline char NS_ToUpper(char aChar)
{
  return (char)nsLowerUpperUtils::kLower2Upper[(unsigned char)aChar];
}

inline char NS_ToLower(char aChar)
{
  return (char)nsLowerUpperUtils::kUpper2Lower[(unsigned char)aChar];
}
  
NS_COM_GLUE bool NS_IsUpper(char aChar);
NS_COM_GLUE bool NS_IsLower(char aChar);

NS_COM_GLUE bool NS_IsAscii(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAscii(const PRUnichar* aString);
NS_COM_GLUE bool NS_IsAsciiAlpha(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAsciiDigit(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAsciiWhitespace(PRUnichar aChar);
NS_COM_GLUE bool NS_IsAscii(const char* aString);
NS_COM_GLUE bool NS_IsAscii(const char* aString, uint32_t aLength);

#ifndef XPCOM_GLUE_AVOID_NSPR
NS_COM_GLUE void NS_MakeRandomString(char *buf, int32_t bufLen);
#endif

#define FF '\f'
#define TAB '\t'

#define CRSTR "\015"
#define LFSTR "\012"
#define CRLF "\015\012"     /* A CR LF equivalent string */

#if defined(XP_MACOSX)
  #define FILE_PATH_SEPARATOR        "/"
  #define OS_FILE_ILLEGAL_CHARACTERS ":"
#elif defined(XP_WIN) || defined(XP_OS2)
  #define FILE_PATH_SEPARATOR        "\\"
  #define OS_FILE_ILLEGAL_CHARACTERS "/:*?\"<>|"
#elif defined(XP_UNIX)
  #define FILE_PATH_SEPARATOR        "/"
  #define OS_FILE_ILLEGAL_CHARACTERS ""
#else
  #error need_to_define_your_file_path_separator_and_illegal_characters
#endif

// Not all these control characters are illegal in all OSs, but we don't really
// want them appearing in filenames
#define CONTROL_CHARACTERS     "\001\002\003\004\005\006\007" \
                           "\010\011\012\013\014\015\016\017" \
                           "\020\021\022\023\024\025\026\027" \
                           "\030\031\032\033\034\035\036\037"

#define FILE_ILLEGAL_CHARACTERS CONTROL_CHARACTERS OS_FILE_ILLEGAL_CHARACTERS

#endif // nsCRTGlue_h__
