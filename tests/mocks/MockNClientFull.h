#pragma once

#include <gmock/gmock.h>
#include <nakama-cpp/NClientInterface.h>
#include <future>

// A concrete mock of Nakama::NClientInterface.
// We only set gmock expectations for methods actually used in tests.
// All other pure virtual methods are given trivial pass-through implementations so
// the class is concrete and instantiable.
class MockNClientFull : public Nakama::NClientInterface {
public:
    MockNClientFull() = default;
    ~MockNClientFull() override = default;

private:
    void* m_userData { nullptr };
    Nakama::ErrorCallback m_defaultErrorCb { nullptr };

    // Helper creators for ready futures.
    template <typename T>
    static std::future<T> makeReadyFuture(T value = T()) {
        std::promise<T> p; p.set_value(std::move(value)); return p.get_future();
    }
    static std::future<void> makeReadyFutureVoid() {
        std::promise<void> p; p.set_value(); return p.get_future();
    }

public:
    // --- Mocked methods used by tests ---
#if !defined(WITH_EXTERNAL_WS) && !defined(BUILD_IO_EXTERNAL)
    MOCK_METHOD(Nakama::NRtClientPtr, createRtClient, (), (override));
#endif
    // Transport variant just delegates to no-arg version (tests don't use transport argument).
    Nakama::NRtClientPtr createRtClient(Nakama::NRtTransportPtr /*transport*/) override {
#if !defined(WITH_EXTERNAL_WS) && !defined(BUILD_IO_EXTERNAL)
        return createRtClient();
#else
        return nullptr;
#endif
    }

    MOCK_METHOD(void, listMatches,
        (Nakama::NSessionPtr session,
         const Nakama::opt::optional<int32_t>& min_size,
         const Nakama::opt::optional<int32_t>& max_size,
         const Nakama::opt::optional<int32_t>& limit,
         const Nakama::opt::optional<std::string>& label,
         const Nakama::opt::optional<std::string>& query,
         const Nakama::opt::optional<bool>& authoritative,
         std::function<void(Nakama::NMatchListPtr)> successCallback,
         Nakama::ErrorCallback errorCallback),
        (override));

    MOCK_METHOD(void, tick, (), (override));
    MOCK_METHOD(void, setUserData, (void*), (override));
    MOCK_METHOD(void*, getUserData, (), (const, override));
    // setUserData/getUserData need backing store for when not explicitly EXPECTed; we supply default actions.

    // Allow tests to run without setting expectations on setUserData/getUserData.
    void SetUpDefaultActions() {
        ON_CALL(*this, setUserData(testing::_)).WillByDefault([this](void* ptr){ m_userData = ptr; });
        ON_CALL(*this, getUserData()).WillByDefault([this](){ return m_userData; });
    }

    // ---- Trivial implementations for the rest of interface ----
    void setErrorCallback(Nakama::ErrorCallback errorCallback) override { m_defaultErrorCb = std::move(errorCallback); }
    void disconnect() override {}

    // Authentication (sync callbacks)
    void authenticateDevice(const std::string&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&,
                            const Nakama::NStringMap&, std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateEmail(const std::string&, const std::string&, const std::string&, bool, const Nakama::NStringMap&,
                           std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateFacebook(const std::string&, const std::string&, bool, bool, const Nakama::NStringMap&,
                              std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateGoogle(const std::string&, const std::string&, bool, const Nakama::NStringMap&,
                            std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateGameCenter(const std::string&, const std::string&, Nakama::NTimestamp, const std::string&, const std::string&, const std::string&, const std::string&, bool, const Nakama::NStringMap&,
                                std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateApple(const std::string&, const std::string&, bool, const Nakama::NStringMap&,
                           std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateCustom(const std::string&, const std::string&, bool, const Nakama::NStringMap&,
                            std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateSteam(const std::string&, const std::string&, bool, const Nakama::NStringMap&,
                           std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void authenticateRefresh(Nakama::NSessionPtr, std::function<void(Nakama::NSessionPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }

    // Linking (sync)
    void linkFacebook(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<bool>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkEmail(Nakama::NSessionPtr, const std::string&, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkDevice(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkGoogle(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkGameCenter(Nakama::NSessionPtr, const std::string&, const std::string&, Nakama::NTimestamp, const std::string&, const std::string&, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkApple(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkSteam(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void linkCustom(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }

    void unlinkFacebook(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkEmail(Nakama::NSessionPtr, const std::string&, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkGoogle(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkGameCenter(Nakama::NSessionPtr, const std::string&, const std::string&, Nakama::NTimestamp, const std::string&, const std::string&, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkApple(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkSteam(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkDevice(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void unlinkCustom(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }

    void importFacebookFriends(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<bool>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }

    // Account / users (sync)
    void getAccount(Nakama::NSessionPtr, std::function<void(const Nakama::NAccount&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NAccount()); }
    void updateAccount(Nakama::NSessionPtr, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void getUsers(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&, const std::vector<std::string>&, std::function<void(const Nakama::NUsers&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NUsers()); }
    void addFriends(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void deleteFriends(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void blockFriends(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void listFriends(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NFriend::State>&, const std::string&, std::function<void(Nakama::NFriendListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }

    // Groups (sync)
    void createGroup(Nakama::NSessionPtr, const std::string&, const std::string&, const std::string&, const std::string&, bool, const Nakama::opt::optional<int32_t>&, std::function<void(const Nakama::NGroup&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NGroup()); }
    void deleteGroup(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void addGroupUsers(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void listGroupUsers(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NUserGroupState>&, const std::string&, std::function<void(Nakama::NGroupUserListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void kickGroupUsers(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void joinGroup(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void leaveGroup(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void listGroups(Nakama::NSessionPtr, const std::string&, int32_t, const std::string&, std::function<void(Nakama::NGroupListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void listUserGroups(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NUserGroupState>&, const std::string&, std::function<void(Nakama::NUserGroupListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void listUserGroups(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NUserGroupState>&, const std::string&, std::function<void(Nakama::NUserGroupListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void promoteGroupUsers(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void demoteGroupUsers(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void updateGroup(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }

    // Leaderboards / tournaments (sync)
    void listLeaderboardRecords(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NLeaderboardRecordListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void listLeaderboardRecordsAroundOwner(Nakama::NSessionPtr, const std::string&, const std::string&, const Nakama::opt::optional<int32_t>&, std::function<void(Nakama::NLeaderboardRecordListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void writeLeaderboardRecord(Nakama::NSessionPtr, const std::string&, std::int64_t, const Nakama::opt::optional<std::int64_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NLeaderboardRecord)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NLeaderboardRecord()); }
    void writeTournamentRecord(Nakama::NSessionPtr, const std::string&, std::int64_t, const Nakama::opt::optional<std::int64_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NLeaderboardRecord)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NLeaderboardRecord()); }
    void deleteLeaderboardRecord(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }

    void listNotifications(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NNotificationListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void deleteNotifications(Nakama::NSessionPtr, const std::vector<std::string>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void listChannelMessages(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&, std::function<void(Nakama::NChannelMessageListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }

    void listTournaments(Nakama::NSessionPtr, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NTournamentListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void listTournamentRecords(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, const std::vector<std::string>&, std::function<void(Nakama::NTournamentRecordListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void listTournamentRecordsAroundOwner(Nakama::NSessionPtr, const std::string&, const std::string&, const Nakama::opt::optional<int32_t>&, std::function<void(Nakama::NTournamentRecordListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void joinTournament(Nakama::NSessionPtr, const std::string&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }

    // Storage and RPC (sync)
    void listStorageObjects(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NStorageObjectListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void listUsersStorageObjects(Nakama::NSessionPtr, const std::string&, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, std::function<void(Nakama::NStorageObjectListPtr)> success, Nakama::ErrorCallback) override { if (success) success(nullptr); }
    void writeStorageObjects(Nakama::NSessionPtr, const std::vector<Nakama::NStorageObjectWrite>&, std::function<void(const Nakama::NStorageObjectAcks&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NStorageObjectAcks()); }
    void readStorageObjects(Nakama::NSessionPtr, const std::vector<Nakama::NReadStorageObjectId>&, std::function<void(const Nakama::NStorageObjects&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NStorageObjects()); }
    void deleteStorageObjects(Nakama::NSessionPtr, const std::vector<Nakama::NDeleteStorageObjectId>&, std::function<void()> success, Nakama::ErrorCallback) override { if (success) success(); }
    void rpc(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<std::string>&, std::function<void(const Nakama::NRpc&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NRpc()); }
    void rpc(const std::string&, const std::string&, const Nakama::opt::optional<std::string>&, std::function<void(const Nakama::NRpc&)> success, Nakama::ErrorCallback) override { if (success) success(Nakama::NRpc()); }

    // Async variants (futures) -> return ready futures with default values
    std::future<Nakama::NSessionPtr> authenticateDeviceAsync(const std::string&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateEmailAsync(const std::string&, const std::string&, const std::string&, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateFacebookAsync(const std::string&, const std::string&, bool, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateGoogleAsync(const std::string&, const std::string&, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateGameCenterAsync(const std::string&, const std::string&, Nakama::NTimestamp, const std::string&, const std::string&, const std::string&, const std::string&, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateAppleAsync(const std::string&, const std::string&, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateCustomAsync(const std::string&, const std::string&, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateSteamAsync(const std::string&, const std::string&, bool, const Nakama::NStringMap&) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }
    std::future<Nakama::NSessionPtr> authenticateRefreshAsync(Nakama::NSessionPtr) override { return makeReadyFuture<Nakama::NSessionPtr>(nullptr); }

    std::future<void> linkFacebookAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<bool>&) override { return makeReadyFutureVoid(); }
    std::future<void> linkEmailAsync(Nakama::NSessionPtr, const std::string&, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> linkDeviceAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> linkGoogleAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> linkGameCenterAsync(Nakama::NSessionPtr, const std::string&, const std::string&, Nakama::NTimestamp, const std::string&, const std::string&, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> linkAppleAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> linkSteamAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> linkCustomAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }

    std::future<void> unlinkFacebookAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkEmailAsync(Nakama::NSessionPtr, const std::string&, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkGoogleAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkGameCenterAsync(Nakama::NSessionPtr, const std::string&, const std::string&, Nakama::NTimestamp, const std::string&, const std::string&, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkAppleAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkSteamAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkDeviceAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> unlinkCustomAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }

    std::future<void> importFacebookFriendsAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<bool>&) override { return makeReadyFutureVoid(); }

    std::future<Nakama::NAccount> getAccountAsync(Nakama::NSessionPtr) override { return makeReadyFuture<Nakama::NAccount>(Nakama::NAccount()); }
    std::future<void> updateAccountAsync(Nakama::NSessionPtr, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<Nakama::NUsers> getUsersAsync(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&, const std::vector<std::string>&) override { return makeReadyFuture<Nakama::NUsers>(Nakama::NUsers()); }
    std::future<void> addFriendsAsync(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<void> deleteFriendsAsync(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<void> blockFriendsAsync(Nakama::NSessionPtr, const std::vector<std::string>&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<Nakama::NFriendListPtr> listFriendsAsync(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NFriend::State>&, const std::string&) override { return makeReadyFuture<Nakama::NFriendListPtr>(nullptr); }

    std::future<Nakama::NGroup> createGroupAsync(Nakama::NSessionPtr, const std::string&, const std::string&, const std::string&, const std::string&, bool, const Nakama::opt::optional<int32_t>&) override { return makeReadyFuture<Nakama::NGroup>(Nakama::NGroup()); }
    std::future<void> deleteGroupAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> addGroupUsersAsync(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<Nakama::NGroupUserListPtr> listGroupUsersAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NUserGroupState>&, const std::string&) override { return makeReadyFuture<Nakama::NGroupUserListPtr>(nullptr); }
    std::future<void> kickGroupUsersAsync(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<void> joinGroupAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<void> leaveGroupAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }
    std::future<Nakama::NGroupListPtr> listGroupsAsync(Nakama::NSessionPtr, const std::string&, int32_t, const std::string&) override { return makeReadyFuture<Nakama::NGroupListPtr>(nullptr); }
    std::future<Nakama::NUserGroupListPtr> listUserGroupsAsync(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NUserGroupState>&, const std::string&) override { return makeReadyFuture<Nakama::NUserGroupListPtr>(nullptr); }
    std::future<Nakama::NUserGroupListPtr> listUserGroupsAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<Nakama::NUserGroupState>&, const std::string&) override { return makeReadyFuture<Nakama::NUserGroupListPtr>(nullptr); }
    std::future<void> promoteGroupUsersAsync(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<void> demoteGroupUsersAsync(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<void> updateGroupAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&) override { return makeReadyFutureVoid(); }

    std::future<Nakama::NLeaderboardRecordListPtr> listLeaderboardRecordsAsync(Nakama::NSessionPtr, const std::string&, const std::vector<std::string>&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NLeaderboardRecordListPtr>(nullptr); }
    std::future<Nakama::NLeaderboardRecordListPtr> listLeaderboardRecordsAroundOwnerAsync(Nakama::NSessionPtr, const std::string&, const std::string&, const Nakama::opt::optional<int32_t>&) override { return makeReadyFuture<Nakama::NLeaderboardRecordListPtr>(nullptr); }
    std::future<Nakama::NLeaderboardRecord> writeLeaderboardRecordAsync(Nakama::NSessionPtr, const std::string&, std::int64_t, const Nakama::opt::optional<std::int64_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NLeaderboardRecord>(Nakama::NLeaderboardRecord()); }
    std::future<Nakama::NLeaderboardRecord> writeTournamentRecordAsync(Nakama::NSessionPtr, const std::string&, std::int64_t, const Nakama::opt::optional<std::int64_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NLeaderboardRecord>(Nakama::NLeaderboardRecord()); }
    std::future<void> deleteLeaderboardRecordAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }

    std::future<Nakama::NMatchListPtr> listMatchesAsync(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&) override { return makeReadyFuture<Nakama::NMatchListPtr>(nullptr); }
    std::future<Nakama::NNotificationListPtr> listNotificationsAsync(Nakama::NSessionPtr, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NNotificationListPtr>(nullptr); }
    std::future<void> deleteNotificationsAsync(Nakama::NSessionPtr, const std::vector<std::string>&) override { return makeReadyFutureVoid(); }
    std::future<Nakama::NChannelMessageListPtr> listChannelMessagesAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, const Nakama::opt::optional<bool>&) override { return makeReadyFuture<Nakama::NChannelMessageListPtr>(nullptr); }

    std::future<Nakama::NTournamentListPtr> listTournamentsAsync(Nakama::NSessionPtr, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<uint32_t>&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NTournamentListPtr>(nullptr); }
    std::future<Nakama::NTournamentRecordListPtr> listTournamentRecordsAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&, const std::vector<std::string>&) override { return makeReadyFuture<Nakama::NTournamentRecordListPtr>(nullptr); }
    std::future<Nakama::NTournamentRecordListPtr> listTournamentRecordsAroundOwnerAsync(Nakama::NSessionPtr, const std::string&, const std::string&, const Nakama::opt::optional<int32_t>&) override { return makeReadyFuture<Nakama::NTournamentRecordListPtr>(nullptr); }
    std::future<void> joinTournamentAsync(Nakama::NSessionPtr, const std::string&) override { return makeReadyFutureVoid(); }

    std::future<Nakama::NStorageObjectListPtr> listStorageObjectsAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NStorageObjectListPtr>(nullptr); }
    std::future<Nakama::NStorageObjectListPtr> listUsersStorageObjectsAsync(Nakama::NSessionPtr, const std::string&, const std::string&, const Nakama::opt::optional<int32_t>&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NStorageObjectListPtr>(nullptr); }
    std::future<Nakama::NStorageObjectAcks> writeStorageObjectsAsync(Nakama::NSessionPtr, const std::vector<Nakama::NStorageObjectWrite>&) override { return makeReadyFuture<Nakama::NStorageObjectAcks>(Nakama::NStorageObjectAcks()); }
    std::future<Nakama::NStorageObjects> readStorageObjectsAsync(Nakama::NSessionPtr, const std::vector<Nakama::NReadStorageObjectId>&) override { return makeReadyFuture<Nakama::NStorageObjects>(Nakama::NStorageObjects()); }
    std::future<void> deleteStorageObjectsAsync(Nakama::NSessionPtr, const std::vector<Nakama::NDeleteStorageObjectId>&) override { return makeReadyFutureVoid(); }
    std::future<Nakama::NRpc> rpcAsync(Nakama::NSessionPtr, const std::string&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NRpc>(Nakama::NRpc()); }
    std::future<Nakama::NRpc> rpcAsync(const std::string&, const std::string&, const Nakama::opt::optional<std::string>&) override { return makeReadyFuture<Nakama::NRpc>(Nakama::NRpc()); }
};
