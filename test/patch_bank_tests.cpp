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
        PatchBankSet banks;
        check(Patch{}.fbDest == 0, "default is not OP1");
        check(file.replaceWithText(R"({"patches":[{"program":0,"fbDest":7},{"program":1}]})"), "fixture write failed");
        check(banks.loadFromJSON(file.getFullPathName().toStdString()), "JSON load failed");
        check(banks.banks[0][0].fbDest == 7, "OP8 not loaded");
        check(banks.banks[0][1].fbDest == 0, "legacy patch not OP1");
        check(banks.banks[0][0].applyMutation(Mutation{}).fbDest == 7, "mutation loses destination");
        check(banks.saveBankToJSON(file.getFullPathName().toStdString()), "JSON save failed");
        banks.banks[0][0].fbDest = 0;
        check(banks.loadFromJSON(file.getFullPathName().toStdString()), "JSON reload failed");
        check(banks.banks[0][0].fbDest == 7, "destination not round-tripped");
        check(file.replaceWithText(R"({"patches":[{"program":0}]})"), "legacy fixture write failed");
        check(banks.loadFromJSON(file.getFullPathName().toStdString()), "legacy reload failed");
        check(banks.banks[0][0].fbDest == 0, "legacy reload retains old destination");
        banks.setPatchOverride(0, 0, 0x41, 5);
        check(banks.banks[0][0].fbDest == 5, "SysEx override lost");
        banks.setPatchOverride(0, 0, 0x41, 255);
        check(banks.banks[0][0].fbDest == 7, "override not bounded");
        check(file.replaceWithText(R"({"patches":[{"program":0,"fbDest":99},{"program":1,"fbDest":-1}]})"), "invalid fixture write failed");
        check(banks.loadFromJSON(file.getFullPathName().toStdString()), "bounded JSON load failed");
        check(banks.banks[0][0].fbDest == 7 && banks.banks[0][1].fbDest == 0, "JSON destination not bounded");

        // 2つのインスタンスが同じJSONを読み込んでも、編集・SysEx・GMリセット・ステート復元は互いに独立
        check(file.replaceWithText(R"({"patches":[{"program":0,"feedback":10},{"program":1,"feedback":20}]})"), "shared fixture write failed");
        PatchBankSet a, b;
        check(a.loadFromJSON(file.getFullPathName().toStdString()) && b.loadFromJSON(file.getFullPathName().toStdString()), "shared load failed");
        a.originalBanks = a.fileBanks = a.banks;
        b.originalBanks = b.fileBanks = b.banks;
        Patch edited = a.banks[0][0];
        edited.feedback = 99;
        a.setUserPatch(0, 0, edited);
        check(a.banks[0][0].feedback == 99 && a.isUserPatchModified(0, 0), "edit not applied");
        check(b.banks[0][0].feedback == 10 && !b.isUserPatchModified(0, 0), "edit leaked into other instance");
        b.setPatchOverride(0, 1, 0x1F, 77);
        check(b.banks[0][1].feedback == 77 && a.banks[0][1].feedback == 20, "SysEx override leaked");
        b.reset();
        check(b.banks[0][1].feedback == 20, "GM reset did not undo override");
        check(a.banks[0][0].feedback == 99, "GM reset in other instance dropped edit");
        a.setPatchOverride(0, 0, 0x1F, 5);
        a.reset();
        check(a.banks[0][0].feedback == 99, "GM reset dropped UI edit");
        // ステート復元（setStateInformation相当）: bは自分のステートだけを読み込む
        b.restoreFromFiles();
        Patch bEdit = b.banks[0][1];
        bEdit.feedback = 42;
        b.setUserPatch(0, 1, bEdit);
        b.clearUserPatch(0, 0);
        check(a.banks[0][0].defined && a.banks[0][0].feedback == 99 && a.banks[0][1].feedback == 20, "state restore leaked");
        check(!b.banks[0][0].defined && b.isUserPatchModified(0, 0) && b.banks[0][1].feedback == 42, "state restore wrong");
        a.revertUserPatch(0, 0);
        check(a.banks[0][0].feedback == 10 && !a.isUserPatchModified(0, 0), "revert wrong");
        check(&a.getEffectivePatch(5, 1) == &a.banks[0][1] && &b.getEffectivePatch(5, 0) == &b.defaultPatch, "fallback wrong");

        file.deleteFile();
        std::cout << "PASS: fbDest JSON round-trip, missing-field OP1 fallback, mutation, override, bounds, per-instance banks\n";
    } catch (const std::exception& e) {
        file.deleteFile();
        std::cerr << e.what() << '\n';
        return 1;
    }
}
