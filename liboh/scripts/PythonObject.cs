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

        public ConsoleHostOptions Options { get { return _optionsParser.Options; } }
        public ScriptRuntimeSetup RuntimeSetup { get { return _optionsParser.RuntimeSetup; } }


        public ConsoleTest(string[] args) {

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



        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes"), System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        private int RunCommandLine(string []args) {
            Console.WriteLine("helloish");
            Debug.Assert(_engine != null);


            ConsoleOptions consoleOptions = _languageOptionsParser.CommonConsoleOptions;

            ScriptScope scope1 = _engine.CreateScope();
            scope1.SetVariable("x",5);
            scope1.SetVariable("y",2);
            ScriptScope scope2 = _engine.CreateScope();
            scope2.SetVariable("x",6);
            scope2.SetVariable("y",4);
            ScriptSource initsource= _engine.CreateScriptSourceFromString("import test\nmyclass=test.exampleclass(y)\n",SourceCodeKind.Statements);
            ScriptSource source= _engine.CreateScriptSourceFromString("retval=myclass.func(x)",SourceCodeKind.Statements);
            initsource.Execute(scope1);
            initsource.Execute(scope2);
            source.Execute(scope1);
            source.Execute(scope2);
            source.Execute(scope1);
            source.Execute(scope2);
            int result=(int)scope2.GetVariable<int>("retval");
            Console.WriteLine("Final Result {0}",result);
            int exitCode=0;

            return exitCode;
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
