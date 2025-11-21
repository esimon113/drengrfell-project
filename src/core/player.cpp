#include "player.h"
#include "types.h"


namespace df{

    size_t Player::getId() const{
        return this->playerId;
    }

    int Player::getHeroPoints() const{
        return 0;
    }

    void Player::addHeroPoints(int points){
        
    }

    void Player::setHeroPoints(int points){

    }



    int Player::getSettlementCount() const
    {
        return 0;
    }

    const std::vector<Settlement *> &Player::getSettlement() const
    {
        // TODO: insert return statement here
        return  this->settlements;
    }

    void Player::addSettlement(Settlement *settlement){

    }

    void Player::removeSettlement(Settlement *settlement){
    }



    void Player::addResources(types::TileType, int amount){
    }

    void Player::removeResources(types::TileType, int amount){
    }

    int Player::getResources(types::TileType, int amount) const{
        return ;
    }

    bool Player::hasResources(const std::map<types::TileType, int>& amountRequired){
        return false;
    }

    const std::map<types::TileType, int> &Player::getResources() const{
        // TODO: insert return statement here
    }

    void Player::setHero(Hero *hero){
    }

    Hero *Player::getHero() const{
        return nullptr;
    }

    void Player::addRoad(Road *road){
    }

    const std::vector<Road *> &Player::getRoads() const{
        // TODO: insert return statement here
    }

    int Player::getRoadCount() const{
        return 0;
    }

    void Player::exploreTile(Tile *tile){
    }

    bool Player::isTileExplored(const Tile *tile) const{
        return false;
    }

    bool Player::isTileExplored(size_t tileId) const{
        return false;
    }

    const std::vector<Tile *> &Player::getExploredTiles() const{
        // TODO: insert return statement here
    }

    void Player::reset(){
    }

}

