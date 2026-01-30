#pragma once
#include "../Namespaces.h"

namespace Utilities
{
	template <typename Derived>
	class MeyersSingleton
	{
	protected:
		MeyersSingleton() = default;
		~MeyersSingleton() = default;
		
		MeyersSingleton(const MeyersSingleton&) = delete;
		MeyersSingleton& operator=(const MeyersSingleton&) = delete;
		MeyersSingleton(MeyersSingleton&&) = delete;
		MeyersSingleton& operator=(MeyersSingleton&&) = delete;

	public:
		static Derived& getInstance()
		{
			static Derived instance;
			return instance;
		}
	};
}
