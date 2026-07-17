// Single place for all name / app-id / storage constants, so nothing else hardcodes them.
#ifndef GRAIN_APPID_H
#define GRAIN_APPID_H

namespace grain {
namespace AppId {

// Organisation + app identifiers (QSettings, sandbox paths).
static const char* const kOrganization = "harbour-grain";
static const char* const kApplication  = "harbour-grain";

// Human-facing display name.
static const char* const kDisplayName  = "Grain";

// On-disk database file name (stored under the app's local data dir).
static const char* const kDatabaseFile = "grain.sqlite";

// SQLite schema version. Bump when the schema changes; migrations key off this.
static const int kSchemaVersion = 1;

} // namespace AppId
} // namespace grain

#endif // GRAIN_APPID_H
