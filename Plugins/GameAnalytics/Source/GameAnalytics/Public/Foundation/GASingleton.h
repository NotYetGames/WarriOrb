#pragma once

#include <memory>
#include <mutex>

/*!
	Template class, that implements the sharedInstance/setSharedInstance idom.

	T mus be default constructable.

	Usage:

	class C : GASingleton<C> {};

	C::sharedInstance();

	TODO: maybe we should lock the instance holder?
*/
template <class T>
class GASingleton
{
	public:
		static T* sharedInstance()
		{

			{
				static std::mutex instanceMutex;
				std::lock_guard<std::mutex> lock(instanceMutex);
				
				if (!getInstanceHolder())
					getInstanceHolder().reset(new T());
			}
			return getInstanceHolder().get();
			
		}

		static void setSharedInstance(T* instance)
		{
			getInstanceHolder.reset(instance);
		}
	private:
		static std::unique_ptr<T>& getInstanceHolder()
		{
			static std::unique_ptr<T> instance_;
			return instance_;
		}
};
