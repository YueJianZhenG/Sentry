#include"Message/s2s/s2s.pb.h"
#include"Rpc/Service/PhysicalRpcService.h"
namespace Tendo
{
	class WatchDog : public PhysicalRpcService
	{
	public:
		WatchDog() = default;
	private:
		bool OnInit() final;
	private:
		int Ping(const Rpc::Packet& packet);
		int ShowLog(const s2s::log::show& request);
	};
}