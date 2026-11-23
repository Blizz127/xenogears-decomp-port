// Inspect target objects without loading the game or executing their code.
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/Object/COFF.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <optional>
#include <string>

using namespace llvm;
using namespace llvm::object;

namespace {
struct DataSymbol {
    uint64_t offset;
    uint64_t section_size;
    int32_t section;
    bool external;
    std::optional<uint64_t> type_size;
};

bool isIdentifier(StringRef s) {
    if (s.empty()) return false;
    auto first = [](char c) {
        return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    };
    if (!first(s.front())) return false;
    for (char c : s.drop_front())
        if (!first(c) && !(c >= '0' && c <= '9')) return false;
    return true;
}

Error inspect(StringRef path) {
    auto object = ObjectFile::createObjectFile(path);
    if (!object) return object.takeError();
    const auto *coff = dyn_cast<COFFObjectFile>(object->getBinary());
    if (!coff || coff->getMachine() != COFF::IMAGE_FILE_MACHINE_AMD64 || coff->getDOSHeader())
        return createStringError("expected an unlinked AMD64 COFF object");

    std::map<std::string, DataSymbol> symbols;
    for (SymbolRef symbol : coff->symbols()) {
        auto raw = coff->getCOFFSymbol(symbol);
        if (raw.getSectionNumber() <= 0 || raw.isFunctionDefinition() ||
            (!raw.isExternal() && raw.getStorageClass() != COFF::IMAGE_SYM_CLASS_STATIC))
            continue;
        auto section = coff->getSection(raw.getSectionNumber());
        if (!section) return section.takeError();
        uint32_t flags = (*section)->Characteristics;
        if (!(flags & (COFF::IMAGE_SCN_CNT_INITIALIZED_DATA | COFF::IMAGE_SCN_CNT_UNINITIALIZED_DATA)) ||
            (flags & (COFF::IMAGE_SCN_CNT_CODE | COFF::IMAGE_SCN_MEM_EXECUTE |
                      COFF::IMAGE_SCN_MEM_DISCARDABLE)))
            continue;
        auto name = symbol.getName();
        if (!name) return name.takeError();
        if (!isIdentifier(*name)) continue;
        symbols.emplace(name->str(), DataSymbol{raw.getValue(), (*section)->SizeOfRawData,
                                                raw.getSectionNumber(), raw.isExternal(), std::nullopt});
    }

    auto debug = DWARFContext::create(*coff);
    for (const auto &unit : debug->compile_units()) {
        if (unit->getAddressByteSize() != 8)
            return createStringError("invalid Windows x64 DWARF pointer width");
        for (auto variable : unit->getUnitDIE(false).children()) {
            if (variable.getTag() != dwarf::DW_TAG_variable ||
                !variable.find(dwarf::DW_AT_location) ||
                dwarf::toUnsigned(variable.find(dwarf::DW_AT_declaration), 0))
                continue;
            const char *name = variable.getShortName();
            if (!name) continue;
            auto it = symbols.find(name);
            if (it == symbols.end()) continue;
            auto &s = it->second;
            if (s.external != bool(dwarf::toUnsigned(variable.find(dwarf::DW_AT_external), 0)))
                continue;
            auto size = variable.getTypeSize(unit->getAddressByteSize());
            if (!size || !*size) continue;
            if (s.offset > s.section_size || *size > s.section_size - s.offset)
                return createStringError("DWARF object %s exceeds its section", name);
            if (s.type_size && s.type_size != size)
                return createStringError("conflicting definitions for %s", name);
            s.type_size = *size;
        }
    }
    for (const auto &[name, symbol] : symbols)
        outs() << sys::path::filename(path) << '\t' << name << '\t' << symbol.section << '\t'
               << symbol.offset << '\t' << symbol.external << '\t' << symbol.type_size.value_or(0) << '\n';
    return Error::success();
}
}

int main(int argc, char **argv) {
    if (argc < 2) {
        errs() << "usage: coff_data_records object.o ...\n";
        return 1;
    }
    for (int i = 1; i < argc; ++i)
        if (auto error = inspect(argv[i])) {
            logAllUnhandledErrors(std::move(error), errs(), Twine(argv[i]) + ": ");
            return 1;
        }
    return 0;
}
