/** Utilities for escaping/unescaping values so they can be used as
 *  literals in strings which will eventually be passed to eval.
 */

if (typeof(Escape) === "undefined") Escape = {};

/** Escape a string.
 *  \param {string} orig the original string
 *  \param {string} quot the quote character to wrap the string in.
 *  \returns {string} the original string in a form suitable for use
 *  as a literal, wrapped in the specified quotes.
 */
Escape.escapeString = function(orig, quot) {
    if (quot != "'" && quot != '"')
        throw "Only \" and ' are supported quotes.";

    var res = '';
    res += quot;

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
        else {
            res += ch;
        }
    }

    res += quot;

    return res;
};
