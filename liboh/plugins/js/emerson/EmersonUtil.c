#include "EmersonUtil.h"

extern pANTLR3_UINT8  EmersonParserTokenNames[];

pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree)
{
  pANTLR3_STRING  string;
  ANTLR3_UINT32   i;
  ANTLR3_UINT32   n;
	 pANTLR3_BASE_TREE   t;


		if	(tree->children == NULL || tree->children->size(tree->children) == 0)
  {
    return	tree->toString(tree);
  }
  
		// THis is how you get a new string. The string is blank

  string	= tree->strFactory->newRaw(tree->strFactory);

  if	(tree->isNilNode(tree) == ANTLR3_FALSE)
		{
		  string->append8	(string, "(");
				pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
    ANTLR3_UINT32 type = token->type;
				string->append(string, EmersonParserTokenNames[type]);
	   string->append8	(string, " ");
  }

  if	(tree->children != NULL)
		{
		  n = tree->children->size(tree->children);
				for	(i = 0; i < n; i++)
				{   
				  t   = (pANTLR3_BASE_TREE) tree->children->get(tree->children, i);

						if  (i > 0)
						{
						  string->append8(string, " ");
						}
						string->appendS(string, emerson_printAST(t));
					}
			}
   
			if	(tree->isNilNode(tree) == ANTLR3_FALSE)
				{
						string->append8(string,")");
				}

				return  string;

}
