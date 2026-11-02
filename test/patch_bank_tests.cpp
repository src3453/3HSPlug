#include "../src/PatchBankData.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <stdexcept>

static void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

int main() {
    const auto file = juce::File::getCurrentWorkingDirectory().getChildFile("fb-dest-test.json");
    try {
        check(Patch{}.fbDest == 0, "default is not OP1");
        check(file.replaceWithText(R"({"patches":[{"program":0,"fbDest":7},{"program":1}]})"), "fixture write failed");
        check(loadPatchBankFromJSON(file.getFullPathName().toStdString()), "JSON load failed");
        check(PatchBanks[0][0].fbDest == 7, "OP8 not loaded");
        check(PatchBanks[0][1].fbDest == 0, "legacy patch not OP1");
        check(PatchBanks[0][0].applyMutation(Mutation{}).fbDest == 7, "mutation loses destination");
        check(savePatchBankToJSON(file.getFullPathName().toStdString()), "JSON save failed");
        PatchBanks[0][0].fbDest = 0;
        check(loadPatchBankFromJSON(file.getFullPathName().toStdString()), "JSON reload failed");
        check(PatchBanks[0][0].fbDest == 7, "destination not round-tripped");
        check(file.replaceWithText(R"({"patches":[{"program":0}]})"), "legacy fixture write failed");
        check(loadPatchBankFromJSON(file.getFullPathName().toStdString()), "legacy reload failed");
        check(PatchBanks[0][0].fbDest == 0, "legacy reload retains old destination");
        setPatchOverride(0, 0, 0x41, 5);
        check(PatchBanks[0][0].fbDest == 5, "SysEx override lost");
        setPatchOverride(0, 0, 0x41, 255);
        check(PatchBanks[0][0].fbDest == 7, "override not bounded");
        check(file.replaceWithText(R"({"patches":[{"program":0,"fbDest":99},{"program":1,"fbDest":-1}]})"), "invalid fixture write failed");
        check(loadPatchBankFromJSON(file.getFullPathName().toStdString()), "bounded JSON load failed");
        check(PatchBanks[0][0].fbDest == 7 && PatchBanks[0][1].fbDest == 0, "JSON destination not bounded");
        file.deleteFile();
        std::cout << "PASS: fbDest JSON round-trip, missing-field OP1 fallback, mutation, override, bounds\n";
    } catch (const std::exception& e) {
        file.deleteFile();
        std::cerr << e.what() << '\n';
        return 1;
    }
}
