#include "Util.h"
#include <antlr3.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;


keymap* emerson_sysKeys;

// Initialize emerson compiler
int emerson_init()
{
  emerson_sysKeys = new keymap();
  insertKeywords();
  return 1;
}


void insertKeywords()
{
  //cout << "insertKeywords() called\n";
  emerson_sysKeys->insert(keypair("print", "print"));
  emerson_sysKeys->insert(keypair("presences", "presences"));
}


int emerson_isImaginaryToken(pANTLR3_COMMON_TOKEN token)
{
  return 0;
}




void emerson_printRewriteStream(pANTLR3_REWRITE_RULE_TOKEN_STREAM rwStream)
{
  pANTLR3_BASE_TREE tree = (pANTLR3_BASE_TREE)rwStream->_next(rwStream);
  while(tree != NULL)
  {
      pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
      if( !(emerson_isImaginaryToken(token)))
      {
          printf((const char*)token->getText(token)->chars);
      }

      tree = (pANTLR3_BASE_TREE)rwStream->_next(rwStream);
  }
  return;
}


pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree, pANTLR3_UINT8* parserTokenNames)
{
  pANTLR3_STRING  string;
  ANTLR3_UINT32   i;
  ANTLR3_UINT32   n;
  pANTLR3_BASE_TREE   t;


  if(tree->children == NULL || tree->children->size(tree->children) == 0)
  {
      return	tree->toString(tree);
  }

  // THis is how you get a new string. The string is blank

  string = tree->strFactory->newRaw(tree->strFactory);

  if(tree->isNilNode(tree) == ANTLR3_FALSE)
  {
      string->append8	(string, "(");
      pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
      ANTLR3_UINT32 type = token->type;

      string->append(string, (const char*)parserTokenNames[type]);
      string->append8(string, " ");
  }

  if(tree->children != NULL)
  {
      n = tree->children->size(tree->children);
      for	(i = 0; i < n; i++)
      {
          t   = (pANTLR3_BASE_TREE) tree->children->get(tree->children, i);
          
          if  (i > 0)
          {
              string->append8(string, " ");
          }
          string->appendS(string, emerson_printAST(t,parserTokenNames));
      }
  }
  
  if(tree->isNilNode(tree) == ANTLR3_FALSE)
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



char* read_file(char* filename)
{
    char * output = new char[20000];
    //strcpy(output, "\n\n");
    ifstream myfile;
    myfile.open (filename);
    string line;
    if (myfile.is_open())
    {
        while (! myfile.eof() )
        {
            getline (myfile,line);
            strcat(output, line.c_str());
            strcat(output, "\n");
        }
        myfile.close();
        return output;
    }
    else
    {
        cout << "Could not open the file " << filename << endl;
    }
    
    return NULL;
}


std::string replaceAllInstances(std::string initialString, std::string toReplace, std::string toReplaceWith)
{
    size_t posToSearchFrom = 0;
    size_t finder   = initialString.find(toReplace, posToSearchFrom);

    while (finder != std::string::npos)
    {
        initialString.replace(finder,toReplace.size(), toReplaceWith);
        posToSearchFrom = finder +  toReplaceWith.size();
        finder = initialString.find(toReplace, posToSearchFrom);
    }
    return initialString;
}


std::string emerson_escapeSingleQuotes(const char* stringSequence)
{
    return replaceAllInstances(std::string(stringSequence),"'","\\'");
}



int  emerson_isAKeyword(const char* word)
{
  // Check if the keyword is one of the emerson keywords

  /*
  if( strcmp(word, "print") == 0)
		{
		  //cout << "word is print\n";

		}
	 keymap::iterator it = emerson_sysKeys->find(word);
	 //keymap::iterator it = emerson_sysKeys->find("print");
  if( it != emerson_sysKeys->end() )
		{
		  cout << "Returning 1\n";
		  return 1;
		}
		*/

  return 0;
}

