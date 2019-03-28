namespace MAT_NS_BEGIN {

    bool TransmitProfiles::load(const std::string&) { return false; };

    void TransmitProfiles::reset() { };

    bool TransmitProfiles::setDefaultProfile(const TransmitProfile) { return false; };

    bool TransmitProfiles::setProfile(const std::string&) { return false; }

    std::string& TransmitProfiles::getProfile() { static std::string def = ""; return def; }

    bool TransmitProfiles::updateStates(NetworkCost, PowerSource) { return true; };

    void TransmitProfiles::getTimers(std::vector<int>& out)
    {
        out = { 1, 2, 4 };
    }

    bool TransmitProfiles::isTimerUpdateRequired() { return false; }

} MAT_NS_END
