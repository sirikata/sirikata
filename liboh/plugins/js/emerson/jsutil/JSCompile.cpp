#include <v8.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include<cassert>
using namespace v8;
using namespace std;


int global_population = 100;

Persistent<Context> current_context;

char* read_js_file_as_string(char* js_file_name)
{
		char * output = new char[2000];
		strcpy(output, "\n\n");
		ifstream myfile;
		myfile.open (js_file_name);
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
			cout << "Could not open the file " << js_file_name << endl;
		}

		return NULL;
}

// The callback that is invoked by v8 whenever the JavaScript 'print'

// function is called. Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++)
    {
        v8::HandleScope handle_scope;
        if (first)
        {
            first = false;
        }
        else
        {
            printf(" ");
        }
        //convert the args[i] type to normal char* string
        v8::String::AsciiValue str(args[i]);
        printf("%s", *str);
    }
    printf("\n");
    //returning Undefined is the same as returning void...
    return v8::Undefined();
}







int main(int argc, char* argv[]) {
	
		
	
	
	// Create a stack-allocated handle scope.
	HandleScope handle_scope;


	// creating a global template

	Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));	
		
	// Create a new context.
	Persistent<Context> context = Context::New(NULL, global);
	

	current_context = context;
	// Enter the created context for compiling and
	// running the hello world script. 
	Context::Scope context_scope(context);

	//Local<Object> global = Context::GetCurrent()->Global();
	
	//...
	//associates plus on script to the Plus function
	
	//create_entity_with_name("bhupesh", context);
	
	
	Handle<String> source ;
	Handle<Script> script;

	// Get the JavaScript file as a string 
	// arg[0] is the file name

	if( argv[1] == NULL)
	{
		// Create a string containing the JavaScript source code.
		source = String::New("'Hello' + ', World!'");
		script = Script::Compile(source);
	}
	else
	{
		char* s = read_js_file_as_string(argv[1]);
		cout << "Script is " << endl;
		cout << s << endl;

		source = String::New(s);
		script = Script::Compile(source, String::New(argv[1]));
   }

	// Run the script to get the result.
//	script->Run();
	
//	assert(f_value->IsFunction());
//	Local<Function> f = Local<Function>::Cast(f_value);
//	f->Call(Context::GetCurrent()->Global(), 0, NULL);

	//Local<Value> args[2] = {v8::Number::New(3), v8::Number::New(4)};
	
	

	//Local<Number> result = (f->Call(Context::GetCurrent()->Global(), 2, args))->ToNumber();
	
	
	//int num = result->Int32Value();
	//printf("%d\n", num);
		 
	
	// Dispose the persistent context.
	context.Dispose();
	
	
	return 0;
}



