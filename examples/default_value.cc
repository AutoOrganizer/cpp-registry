#include <cpp-registry/Registry.h>

int main() {
	using namespace kare;

	RegistryKey key = Registry::currentUser.createSubKey("Software\\cpp-registry-default-demo");

	key.setValue("", 2026);

	RegistryValue value         = key.getValue("");
	int           defaultNumber = value.dWordValue();
	(void)defaultNumber;

	key[""]              = 2048;
	RegistryValue value2 = key[""];
	(void)value2;

	Registry::deleteKey("HKCU\\Software\\cpp-registry-default-demo");
	return 0;
}
