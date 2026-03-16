#include <iostream>

#include <cpp-registry/Registry.h>
#include <cpp-registry/RegistryIterator.h>

int main() {
	using namespace kare;

	RegistryKey root = Registry::currentUser.createSubKey("Software\\cpp-registry-iter-demo");
	root.createSubKey("A");
	root.createSubKey("B");
	root.setValue("Enabled", 1);

	for (const auto& sub : root.subKeys()) {
		std::cout << "subkey: " << sub.getName() << "\n";
	}

	for (const auto& item : root.values()) {
		std::cout << "value: " << item.name << "\n";
	}

	for (auto it = root.begin(); it != root.end(); ++it) {
		auto& entry = *it;
		if (entry.isKey()) {
			std::cout << "[K] " << entry.key().getName() << "\n";
		} else {
			std::cout << "[V] " << entry.value().name << "\n";
		}
	}

	Registry::deleteKey("HKCU\\Software\\cpp-registry-iter-demo");
	return 0;
}
