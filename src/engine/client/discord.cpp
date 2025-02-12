#include <base/system.h>
#include <engine/discord.h>
#include <map>

#if defined(CONF_DISCORD)
#include <discord_game_sdk.h>

typedef enum EDiscordResult(DISCORD_API *FDiscordCreate)(DiscordVersion, struct DiscordCreateParams *, struct IDiscordCore **);

#if defined(CONF_DISCORD_DYNAMIC)
#include <dlfcn.h>
FDiscordCreate GetDiscordCreate()
{
	void *pSdk = dlopen("discord_game_sdk.so", RTLD_NOW);
	if(!pSdk)
	{
		return nullptr;
	}
	return (FDiscordCreate)dlsym(pSdk, "DiscordCreate");
}
#else
FDiscordCreate GetDiscordCreate()
{
	return DiscordCreate;
}
#endif

struct DiscordApp {
	uint64_t ClientId;
	const char *OnlineDetails;
	const char *OfflineDetails;
	const char *LargeImage;
	const char *LargeText;
};

static const std::map<int, DiscordApp> DISCORD_APPS = {
	// { Id, { ClientId, OnlineDetails, OfflineDetails, LargeImage, LargeText } }
	{0, {1337554226926321745, "Online", "Offline", "zyro_logo", "Zyro logo"}},                                    // Zyro
	{1, {1326161704043679845, "Online", "Offline", "ddnet_logo", "DDNet logo"}},                                  // KRX
	{2, {1121238977228328970, "Online", "Offline", "cff_logo", "CFF logo"}},                                      // CFF
	{3, {752165779117441075, "Online", "Offline", "ddnet_logo", "DDNet logo"}},                                   // DDNet
	{4, {1325361453988970527, "In-Game", "In-Menus", "tclient", "TClient"}},                                      // Tater
	{5, {1142757199249162321, "В игре", "В меню", "ddnet_logo", "DDNet logo"}},                                   // Cactus
	{6, {1325507236331524116, "Prochiral-Gay", "Offline", "ac_image_b", "github.com/qxdFox/Aiodob-Client-DDNet"}} // Aiodob Client
};

class CDiscord : public IDiscord
{
	IDiscordCore *m_pCore;
	IDiscordActivityEvents m_ActivityEvents;
	IDiscordActivityManager *m_pActivityManager;
	int m_CurrentApp;
	bool m_Enabled;
	FDiscordCreate m_pfnDiscordCreate;

public:
	bool Init(FDiscordCreate pfnDiscordCreate)
	{
		m_pfnDiscordCreate = pfnDiscordCreate;
		m_CurrentApp = 0;
		m_Enabled = false;
		return InitDiscord();
	}

	bool InitDiscord()
	{
		if(m_pCore)
		{
			m_pCore->destroy(m_pCore);
			m_pCore = 0;
			m_pActivityManager = 0;
		}

		if(!m_Enabled)
			return false;

		mem_zero(&m_ActivityEvents, sizeof(m_ActivityEvents));
		m_pActivityManager = 0;

		DiscordCreateParams Params;
		DiscordCreateParamsSetDefault(&Params);

		auto it = DISCORD_APPS.find(m_CurrentApp);
		if(it == DISCORD_APPS.end())
			it = DISCORD_APPS.begin();

		Params.client_id = it->second.ClientId;
		Params.flags = EDiscordCreateFlags::DiscordCreateFlags_NoRequireDiscord;
		Params.event_data = this;
		Params.activity_events = &m_ActivityEvents;
		int Error = m_pfnDiscordCreate(DISCORD_VERSION, &Params, &m_pCore);
		if(Error != DiscordResult_Ok)
		{
			dbg_msg("discord", "error initializing discord instance, error=%d", Error);
			return true;
		}

		m_pActivityManager = m_pCore->get_activity_manager(m_pCore);
		ClearGameInfo();
		return false;
	}

	void Update(bool Enabled, int DiscordIndex) override
	{
		bool NeedsUpdate = m_Enabled != Enabled || (Enabled && m_CurrentApp != DiscordIndex);
		m_Enabled = Enabled;
		m_CurrentApp = DiscordIndex;

		if(NeedsUpdate)
			InitDiscord();

		if(m_pCore && m_Enabled)
			m_pCore->run_callbacks(m_pCore);
	}

	void ClearGameInfo() override
	{
		if(!m_Enabled || !m_pActivityManager)
			return;

		DiscordActivity Activity;
		mem_zero(&Activity, sizeof(DiscordActivity));

		auto it = DISCORD_APPS.find(m_CurrentApp);
		if(it == DISCORD_APPS.end())
			it = DISCORD_APPS.begin();

		str_copy(Activity.assets.large_image, it->second.LargeImage, sizeof(Activity.assets.large_image));
		str_copy(Activity.assets.large_text, it->second.LargeText, sizeof(Activity.assets.large_text));
		Activity.timestamps.start = time_timestamp();

		str_copy(Activity.details, it->second.OfflineDetails, sizeof(Activity.details));
		m_pActivityManager->update_activity(m_pActivityManager, &Activity, 0, 0);
	}

	void SetGameInfo(const NETADDR &ServerAddr, const char *pMapName, bool AnnounceAddr) override
	{
		if(!m_Enabled || !m_pActivityManager)
			return;

		DiscordActivity Activity;
		mem_zero(&Activity, sizeof(DiscordActivity));

		auto it = DISCORD_APPS.find(m_CurrentApp);
		if(it == DISCORD_APPS.end())
			it = DISCORD_APPS.begin();

		str_copy(Activity.assets.large_image, it->second.LargeImage, sizeof(Activity.assets.large_image));
		str_copy(Activity.assets.large_text, it->second.LargeText, sizeof(Activity.assets.large_text));
		Activity.timestamps.start = time_timestamp();

		str_copy(Activity.details, it->second.OnlineDetails, sizeof(Activity.details));
		str_copy(Activity.state, pMapName, sizeof(Activity.state));
		m_pActivityManager->update_activity(m_pActivityManager, &Activity, 0, 0);
	}

	~CDiscord()
	{
		if(m_pCore)
			m_pCore->destroy(m_pCore);
	}
};

IDiscord *CreateDiscordImpl()
{
	FDiscordCreate pfnDiscordCreate = GetDiscordCreate();
	if(!pfnDiscordCreate)
	{
		return 0;
	}
	CDiscord *pDiscord = new CDiscord();
	if(pDiscord->Init(pfnDiscordCreate))
	{
		delete pDiscord;
		return 0;
	}
	return pDiscord;
}
#else
IDiscord *CreateDiscordImpl()
{
	return nullptr;
}
#endif

class CDiscordStub : public IDiscord
{
	void Update(bool Enabled, int DiscordIndex) override {}
	void ClearGameInfo() override {}
	void SetGameInfo(const NETADDR &ServerAddr, const char *pMapName, bool AnnounceAddr) override {}
};

IDiscord *CreateDiscord()
{
	IDiscord *pDiscord = CreateDiscordImpl();
	if(pDiscord)
	{
		return pDiscord;
	}
	return new CDiscordStub();
}
