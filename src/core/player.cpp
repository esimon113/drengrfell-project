#include "player.h"
#include "types.h"

#include <algorithm>


namespace df{

    Player::Player()
        : playerId(0), heroPoints(0), heroReference(nullptr)
    {
    }

    Player::Player(size_t id)
        : playerId(id), heroPoints(0), heroReference(nullptr)
    {
    }

    size_t Player::getId() const{
        return this->playerId;
    }

    int Player::getHeroPoints() const{
        return heroPoints;
    }

    void Player::addHeroPoints(int points){
        heroPoints += points;
    }

    void Player::setHeroPoints(int points){
        heroPoints = points;
    }

    const std::vector<size_t> &Player::getSettlementIds() const
    {
        return  settlementIds;
    }

    void Player::addSettlement(size_t settlementId){
        settlementIds.push_back(settlementId);
    }

    void Player::removeSettlement(size_t settlementId){
        settlementIds.erase(std::remove(settlementIds.begin(), settlementIds.end(), settlementId), settlementIds.end());
    }

    void Player::addResources(types::TileType type, int amount){
        resources[type] += amount;
    }

    void Player::removeResources(types::TileType type, int amount){
        // no check is we call first the hasResources function
        resources[type] -= amount;
    }

    int Player::getResources(types::TileType type) const{
        auto it = resources.find(type);
        if(it != resources.end()){
            return it->second;
        }
        return 0;
    }

    bool Player::hasResources(const std::map<types::TileType, int>& amountRequired){
        for(const auto& [type,amount ]: amountRequired){
            if(getResources(type) < amount){
                return false;
            } 
        }
        return true;
    }

    const std::map<types::TileType, int> &Player::getResources() const{
        return resources;
    }

    void Player::setHero(std::shared_ptr<Hero> hero){
        heroReference = hero;
    }

    std::shared_ptr<Hero> Player::getHero() const{
        return heroReference;
    }

    void Player::addRoad(size_t roadId){
        roadIds.push_back(roadId);
    }

    const std::vector<size_t> &Player::getRoadIds() const{
        return roadIds;
    }

    int Player::getRoadCount() const{
        return roadIds.size();
    }

    void Player::exploreTile(size_t tileId){
        if(!isTileExplored(tileId)){
            exploredTileIds.push_back(tileId);
        }
    }

    bool Player::isTileExplored(size_t tileId) const{ 
        return std::find(exploredTileIds.begin(), exploredTileIds.end(), tileId) != exploredTileIds.end();
    }

    const std::vector<size_t> &Player::getExploredTileIds() const{
        return exploredTileIds;
    }

    void Player::reset(){
        heroPoints = 0;
        settlementIds.clear();
        resources.clear();
        heroReference = nullptr;
        roadIds.clear();
        exploredTileIds.clear();
    }

    size_t Player::getPlayerId() const { return playerId; }
    void Player::setPlayerId(size_t newPlayerId) { playerId = newPlayerId; }

    const json Player::serialize() const {
        
        json j;

        j["playerId"] = playerId;
        j["heroPoints"] = heroPoints;
        
        j["settlementIds"] = settlementIds;
        j["roadIds"] = roadIds;
        j["exploredTileIds"] = exploredTileIds;
        
        // Resources
        json resourcesJson;
        for(const auto& [type, amount] : resources) {
            resourcesJson[std::to_string(static_cast<int>(type))] = amount;
        }
        j["resources"] = resourcesJson;

        if (heroReference) {
            // Uncomment this when hero serialization is implemented
            //j["hero"] = heroReference->serialize();
        }

        return j;
    }

    void Player::deserialize(const json& j) {

        if(j.contains("playerId")) this->setPlayerId(j.at("playerId").get<size_t>());
        if(j.contains("heroPoints")) this->setHeroPoints(j.at("heroPoints").get<int>());
        
        if(j.contains("settlementIds")) settlementIds = j["settlementIds"].get<std::vector<size_t>>();
        if(j.contains("roadIds")) roadIds = j["roadIds"].get<std::vector<size_t>>();
        if(j.contains("exploredTileIds")) exploredTileIds = j["exploredTileIds"].get<std::vector<size_t>>();
        
        if(j.contains("resources")) {
            for(const auto& item : j["resources"].items()) {
                types::TileType type = static_cast<types::TileType>(std::stoi(item.key()));
                resources[type] = item.value().get<int>();
            }
        }

        if (j.contains("hero")) {
            auto hero = std::make_shared<Hero>();
            // UNcomment this when hero deserialization is implemented
            //hero->deserialize(j["hero"]);
            this->setHero(hero);
        }
    }

}