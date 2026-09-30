# Ghidra script which parses splat symbol files

# Possible improvements:
# - Import struct definitions
# - Look up function signature if there is one, and set it

from ghidra.program.model.symbol import SourceType
from ghidra.app.cmd.function import CreateFunctionCmd
from ghidra.program.model.address import AddressSet

def addr_to_str(addr):
    addr = str(hex(addr))
    if addr[-1] == 'L':
        addr = addr[0:-1]
    return addr

def define_function(func_addr, name):
    func_addr = addr_to_str(func_addr)
    func_addr = currentProgram.getAddressFactory().getAddress(func_addr)
    functionManager = currentProgram.getFunctionManager()
    func = functionManager.getFunctionAt(func_addr)
    if func is None:
        body = AddressSet(func_addr)
        cmd = CreateFunctionCmd(name, func_addr, body, SourceType.USER_DEFINED)
        runCommand(cmd)
    else:
        func.setName(name, SourceType.USER_DEFINED)

def define_symbol(addr, name):
    addr = addr_to_str(addr)
    addr = currentProgram.getAddressFactory().getAddress(addr)
    currentProgram.getSymbolTable().createLabel(addr, name, SourceType.USER_DEFINED)

def parse_symbol_file(f):
    for line in file(f.absolutePath):
        # Normalize line
        line = line.strip()
        line = line.replace(" ", "")
        if line.startswith("//") or len(line) < 4:
            continue
        
        # Parse symbol name and address
        parts = line.split("=")
        name = parts[0]
        addr = int(parts[1].split(";")[0], 16)
        print(name, hex(addr))

        if name.startswith("g_"):
            define_symbol(addr, name)
        else:
            define_function(addr, name)

f = askFile("Choose a splat symbol file", "")
parse_symbol_file(f)