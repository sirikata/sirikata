/* ****************************************************************************
 *
 * Copyright (c) Microsoft Corporation. 
 *
 * This source code is subject to terms and conditions of the Microsoft Public License. A 
 * copy of the license can be found in the License.html file at the root of this distribution. If 
 * you cannot locate the  Microsoft Public License, please send an email to 
 * dlr@microsoft.com. By using this source code in any fashion, you are agreeing to be bound 
 * by the terms of the Microsoft Public License.
 *
 * You must not remove this notice, or any other, from this software.
 *
 *
 * ***************************************************************************/

using System; using Microsoft;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using Microsoft.Runtime.CompilerServices;

using System.Text;
using System.Threading;
using System.Collections.Generic;

using Microsoft.Scripting.Ast;
using Microsoft.Scripting.Runtime;
using Microsoft.Scripting.Hosting;
using Microsoft.Scripting.Generation;
using Microsoft.Scripting.Utils;
using Microsoft.Scripting;


using IronPython.Hosting;
using IronPython.Runtime;

using Microsoft.Scripting.Hosting.Shell;


public class PythonObject {

        private ConsoleHostOptionsParser _optionsParser;
        private ScriptRuntime _runtime;
        private ScriptEngine _engine;
        private OptionsParser _languageOptionsParser;
        private ScriptScope _scope;
        private ScriptSource _processMessageSource;
        private ScriptSource _processRPCSource;
        private ScriptSource _processTickSource;
        private object _pythonObject;
        public object PyObject  { get { return _pythonObject; } }
        public ConsoleHostOptions Options { get { return _optionsParser.Options; } }
        public ScriptRuntimeSetup RuntimeSetup { get { return _optionsParser.RuntimeSetup; } }
    

        public PythonObject(string[] args) {

            ScriptRuntimeSetup runtimeSetup = ScriptRuntimeSetup.ReadConfiguration();
            ConsoleHostOptions options = new ConsoleHostOptions();
            _optionsParser = new ConsoleHostOptionsParser(options, runtimeSetup);

            string provider = GetLanguageProvider(runtimeSetup);

            LanguageSetup languageSetup = null;
            foreach (LanguageSetup language in runtimeSetup.LanguageSetups) {
                if (language.TypeName == provider) {
                    languageSetup = language;
                }
            }
            if (languageSetup == null) {
                // the language doesn't have a setup -> create a default one:
                languageSetup = new LanguageSetup(Provider.AssemblyQualifiedName, Provider.Name);
                runtimeSetup.LanguageSetups.Add(languageSetup);
            }

            // inserts search paths for all languages (/paths option):
            InsertSearchPaths(runtimeSetup.Options, Options.SourceUnitSearchPaths);
            
            _languageOptionsParser = new OptionsParser<ConsoleOptions>();

            try {
                _languageOptionsParser.Parse(Options.IgnoredArgs.ToArray(), runtimeSetup, languageSetup, PlatformAdaptationLayer.Default);
            } catch (InvalidOptionException e) {
                Console.Error.WriteLine(e.Message);
            }

            _runtime = new ScriptRuntime(runtimeSetup);

            try {
                _engine = _runtime.GetEngineByTypeName(provider);
            } catch (Exception e) {
                Console.Error.WriteLine(e.Message);
            }
            _scope= _engine.CreateScope();
            RunCommandLine(args);
        }


        #region Customization

        protected virtual LanguageSetup CreateLanguageSetup() {
            return Python.CreateLanguageSetup(null);
        }

        protected virtual Type Provider {
            get { return typeof(PythonContext); }
        }

        private string GetLanguageProvider(ScriptRuntimeSetup setup) {
            Type providerType = Provider;
            if (providerType != null) {
                return providerType.AssemblyQualifiedName;
            }
            
            if (Options.HasLanguageProvider) {
                return Options.LanguageProvider;
            }

            throw new InvalidOptionException("No language specified.");
        }



        #endregion


        private static void InsertSearchPaths(IDictionary<string, object> options, ICollection<string> paths) {
            if (options != null && paths != null && paths.Count > 0) {
                List<string> existingPaths = new List<string>(LanguageOptions.GetSearchPathsOption(options) ?? (IEnumerable<string>)ArrayUtils.EmptyStrings);
                existingPaths.InsertRange(0, paths);
                options["SearchPaths"] = existingPaths;
            }
        }

        private static bool IsSanitized(string inputStr ) {
            if (string.IsNullOrEmpty(inputStr))
                return false;
            for (int i = 0; i < inputStr.Length; i++)
            {
                if (!(char.IsLetter(inputStr[i])) && (!(char.IsNumber(inputStr[i]))))
                    if (inputStr[i]!='.'&&inputStr[i]!='_')
                        return false;
            }
            return true;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes"), System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        private void RunCommandLine(string []args) {
            Debug.Assert(_engine != null);
            string arglist="";
            string python_module="test";
            string python_class="exampleclass";
            for (int i=0;i+1<args.Length;i+=2) {
                if (IsSanitized(args[i+1])) {
                    if (args[i]=="PythonModule") {
                        python_module=args[i+1];
                    }else if (args[i]=="PythonClass") {
                        python_class=args[i+1];
                    }else {
                        _scope.SetVariable(args[i],args[i+1]);
                        if (arglist.Length!=0)
                            arglist+=",";
                        arglist+=args[i]+"="+args[i+1];
                    }
                }
            }
            ScriptSource initsource=_engine.CreateScriptSourceFromString("from "+python_module+" import "+python_class+";retval="+python_class+"("+arglist+");",SourceCodeKind.Statements);
            initsource.Execute(_scope);
            _pythonObject=_scope.GetVariable("retval");
            _processMessageSource=_engine.CreateScriptSourceFromString("retval.processMessage(header,body)",SourceCodeKind.Expression);
            _processRPCSource=_engine.CreateScriptSourceFromString("retval.processRPC(header,name,args)",SourceCodeKind.Expression);
            _processTickSource=_engine.CreateScriptSourceFromString("retval.tick(curTime)",SourceCodeKind.Expression);
        }
        public static object CreatePythonObject(string[]args) {
            return new PythonObject(args)._pythonObject;
        }
        public virtual void processMessage(byte[] header, byte[] body) {
            _scope.SetVariable("header",header);
            _scope.SetVariable("body",body);
            _scope.SetVariable("retval",_pythonObject);
            _processMessageSource.Execute(_scope);
        }
        public virtual Array processRPC(byte[] header, string name, byte[] args) {
            _scope.SetVariable("header",header);
            _scope.SetVariable("name",name);
            _scope.SetVariable("args",args);
            _scope.SetVariable("retval",_pythonObject);
            Console.WriteLine("Processing RPC "+name);
            return _processRPCSource.Execute<Array>(_scope);
        }
        public virtual void tick(System.DateTime time) {
            _scope.SetVariable("curTime",time);
            _scope.SetVariable("retval",_pythonObject);
            _processTickSource.Execute(_scope);
        }
        protected virtual void UnhandledException(ScriptEngine engine, Exception e) {
            Console.Error.Write("Unhandled exception");
            Console.Error.WriteLine(':');

            ExceptionOperations es = engine.GetService<ExceptionOperations>();
            Console.Error.WriteLine(es.FormatException(e));
        }

        protected static void PrintException(TextWriter output, Exception e) {
            Debug.Assert(output != null);
            ContractUtils.RequiresNotNull(e, "e");

            while (e != null) {
                output.WriteLine(e);
                e = e.InnerException;
            }
        }
}
