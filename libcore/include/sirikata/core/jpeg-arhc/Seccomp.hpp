namespace Sirikata {
/// only allow read/write/exit/sigreturn  Return false if unsupported OS
bool installStrictSyscallFilter(bool verbose = false);
}
