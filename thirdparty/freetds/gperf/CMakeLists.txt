set(GPERF_URL "https://ftp.gnu.org/gnu/gperf/gperf-3.3.tar.gz")
set(GPERF_HASH
	"SHA256=fd87e0aba7e43ae054837afd6cd4db03a3f2693deb3619085e6ed9d8d9604ad8")

buildmaster_component(
	gperf
	"GNU gperf 3.3 code generator"
	"${CMAKE_CURRENT_LIST_DIR}/cmake-port"
	""
	executable
	gperf
	"FILES={URL=${GPERF_URL};NAME=gperf;UNPACK;TITLE=GNU gperf 3.3;EXPECTED_HASH=${GPERF_HASH}}"
)
