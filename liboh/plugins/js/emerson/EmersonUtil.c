#include "EmersonUtil.h"
#include <antlr3.h>

extern pANTLR3_UINT8  EmersonParserTokenNames[];


int emerson_isImaginaryToken(pANTLR3_COMMON_TOKEN token)
{
  return 0;
}




void emerson_printRewriteStream(pANTLR3_REWRITE_RULE_TOKEN_STREAM rwStream)
{
  pANTLR3_BASE_TREE tree = rwStream->_next(rwStream);
  while(tree != NULL)
		{
		  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
		  if( !(emerson_isImaginaryToken(token))) 
				{
				  printf(token->getText(token)->chars);
		  }

    tree = rwStream->_next(rwStream);
		}
  return;
}

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

void emerson_createTreeMirrorImage2(pANTLR3_BASE_TREE ptr)
{
  
  if(ptr!= NULL && ptr->children != NULL)
  {

				ANTLR3_UINT32 n = ptr->getChildCount(ptr);
				if(n == 1)
				{
				  //emerson_createTreeMirrorImage((pANTLR3_BASE_TREE)(ptr->getChild(ptr, 0)));
				}
				if(n == 2)  // should it be checked
				{
				  pANTLR3_BASE_TREE right = (pANTLR3_BASE_TREE)(ptr->getChild(ptr, 1));
		    //emerson_createTreeMirrorImage( (pANTLR3_BASE_TREE)(ptr->getChild(ptr, 0)));
			   //emerson_createTreeMirrorImage( (pANTLR3_BASE_TREE)(ptr->getChild(ptr, 1)) );
				  ptr->setChild(ptr, 1, ptr->getChild(ptr, 0));
						ptr->setChild(ptr, 0, right);
				}		
		}


}


void emerson_createTreeMirrorImage(pANTLR3_BASE_TREE ptr)
{
  
  if(ptr!= NULL && ptr->children != NULL)
  {

				ANTLR3_UINT32 n = ptr->getChildCount(ptr);
				if(n == 1)
				{
				  emerson_createTreeMirrorImage((pANTLR3_BASE_TREE)(ptr->getChild(ptr, 0)));
				}
				if(n == 2)  // should it be checked
				{
				  pANTLR3_BASE_TREE right = (pANTLR3_BASE_TREE)(ptr->getChild(ptr, 1));
		    emerson_createTreeMirrorImage( (pANTLR3_BASE_TREE)(ptr->getChild(ptr, 0)));
			   emerson_createTreeMirrorImage( (pANTLR3_BASE_TREE)(ptr->getChild(ptr, 1)) );
				  ptr->setChild(ptr, 1, ptr->getChild(ptr, 0));
						ptr->setChild(ptr, 0, right);
				}		
		}


}

