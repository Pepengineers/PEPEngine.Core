#include "pch.h"
#include "Job.h"

namespace PEPEngine
{
	namespace JobSystem
	{
		void JobInfo::Execute()
		{
			if (!IsNull())
			{
				GetDelegate()->Call();
			}

			if (_counter)
			{
				_counter->Decrement();
			}
		}
	}
}
