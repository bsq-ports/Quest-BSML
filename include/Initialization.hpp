#pragma once
namespace BSML::Internal {
// Loader entrypoints are globally exported and may be interposed by another mod.
// In-library callers must bind directly to these private helpers.
__attribute__((visibility("hidden"))) void Initialize();
__attribute__((visibility("hidden"))) void LateInitialize();
}
