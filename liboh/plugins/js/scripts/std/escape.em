/** Utilities for escaping/unescaping values so they can be used as
 *  literals in strings which will eventually be passed to eval.
 */

if (typeof(Escape) === "undefined") /** @namespace */ Escape = {};

/** Escape a string.
 *  \param {string} orig the original string
 *  \param {string} quot the quote character to wrap the string in. If
 *                  null, doesn't wrap in anything.
 *  \returns {string} the original string in a form suitable for use
 *  as a literal, wrapped in the specified quotes.
 */
Escape.escapeString = function(orig, quot) {
    if (quot === undefined)
        quot = '"';

    if (quot != "'" && quot != '"' && quot != '@' && quot != null)
        throw "Only \", ', and @ are supported quotes.";

    var res = '';
    if (quot) res += quot;

    for (var i = 0; i < orig.length; ++i )
    {
        var ch = orig.charAt(i);

        if (ch == quot) {
            res += "\\" + quot;
        }
        else if (ch == '\n') {
            res += "\\n";
        }
        else if (ch == '\t') {
            res += "\\t";
        }
        else if (ch == '\b') {
            res += "\\b";
        }
        else if (ch == '\f') {
            res += "\\f";
        }
        else if (ch == '\0') {
            res += "\\0";
        }
        else if (ch == '\r') {
            res += "\\r";
        }
        else if (ch == '\v') {
            res += "\\v";
        }
        else if (ch == '\\') {
            res += "\\\\";
        }
        else {
            res += ch;
        }
    }

    if (quot) res += quot;

    return res;
};
