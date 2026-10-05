#include <StormByte/database/value.hxx>
#include <StormByte/database/rows.hxx>
#include <StormByte/safe/optional.hxx>
#include <StormByte/safe/vector.hxx>

#include <string>

#if defined(STORMBYTE_TEST_SQLITE)
#include <StormByte/database/sqlite/sqlite3.hxx>
#endif

class ConsumerDatabase
#if defined(STORMBYTE_TEST_SQLITE)
	: public StormByte::Database::SQLite::SQLite3
#endif
{
#if defined(STORMBYTE_TEST_SQLITE)
	public:
		ConsumerDatabase() : SQLite3(StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}
		ConsumerDatabase(const std::filesystem::path& path)
			: SQLite3(path, StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}
		ConsumerDatabase(std::filesystem::path&& path)
			: SQLite3(std::move(path), StormByte::Safe::Shared<StormByte::Logger::Log>{}) {}
		void RegisterStatement(std::string_view name, std::string_view query) {
			DoPrepareSTMT(name, query);
		}
#endif
};

#if defined(STORMBYTE_TEST_SQLITE)
STORMBYTE_DECLARE_MAYBE_SAFE(ConsumerDatabase);
static_assert(StormByte::Type::MaybeSafe<StormByte::Database::SQLite::SQLite3>);
static_assert(StormByte::Type::SafeComponent<StormByte::Safe::Shared<ConsumerDatabase>>);
#endif

static_assert(StormByte::Type::MaybeSafe<StormByte::Database::Value>);
static_assert(StormByte::Type::SafeValue<StormByte::Database::Rows>);

int main() {
	StormByte::Database::Value value{42};
	if (value.Get<int>() != 42)
		return 1;
	StormByte::Safe::Optional<StormByte::Database::Value> stored{value};
	if (!stored || stored.value().Get<int>() != 42)
		return 8;

#if defined(STORMBYTE_TEST_SQLITE)
	ConsumerDatabase db;
	if (!db.Connect())
		return 2;
	const auto rows = db.Query("SELECT 42;");
	if (!rows || rows->Count() != 1 || rows->operator[](0)[0].Get<int>() != 42)
		return 3;
	const auto telemetry = db.Telemetry();
	if (!telemetry || telemetry->Metrics(StormByte::Database::Operation::Query).Successes != 1)
		return 4;
	const auto* sqlite_telemetry = dynamic_cast<const StormByte::Database::SQLite::Telemetry*>(telemetry.get());
	if (!sqlite_telemetry || static_cast<std::string>(*telemetry).find("SQLite{") == std::string::npos)
		return 5;
	if (sqlite_telemetry->Metrics(StormByte::Database::Operation::Query).Attempts != 1)
		return 6;
	auto transaction = db.BeginTransaction();
	if (!transaction)
		return 7;
	transaction->Rollback();
	StormByte::Safe::Vector<StormByte::Database::Rows> snapshots{*rows};
	const StormByte::Database::Rows snapshot = snapshots[0];
	if (snapshots.size() != 1 || snapshot[0][0].Get<int>() != 42)
		return 9;
	const std::filesystem::path borrowed_path{":memory:"};
	ConsumerDatabase borrowed{borrowed_path};
	if (!borrowed.Connect() || borrowed_path != ":memory:")
		return 10;
	ConsumerDatabase temporary{std::filesystem::path{":memory:"}};
	if (!temporary.Connect())
		return 11;
	constexpr std::string_view text{"left\0right", 10};
	constexpr std::string_view name{"echo\0suffix", 11};
	db.RegisterStatement(name, "SELECT ?;");
	const auto full_text = db.ExecuteSTMT(name, StormByte::Safe::String{text});
	if (!full_text || static_cast<std::string_view>((*full_text)[0][0].Get<StormByte::Safe::String>()) != text)
		return 12;
	StormByte::Safe::Vector<StormByte::Database::Rows> text_snapshots{*full_text};
	const StormByte::Database::Rows text_snapshot = text_snapshots[0];
	if (static_cast<std::string_view>(text_snapshot[0][0].Get<StormByte::Safe::String>()) != text)
		return 13;
#endif
	return 0;
}
