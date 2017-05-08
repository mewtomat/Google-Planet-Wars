#include "PlanetWars.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm> 
#include <fstream>

using namespace std;
std::ofstream pwlog;

std::vector<std::vector<int> > distance_;
int flip(int a)
  {
    if(a==1) return 2;
    if(a==2) return 1;
  }

void StringUtil::Tokenize(const std::string& s,
                          const std::string& delimiters,
                          std::vector<std::string>& tokens) {
  std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
  std::string::size_type pos = s.find_first_of(delimiters, lastPos);
  while (std::string::npos != pos || std::string::npos != lastPos) {
    tokens.push_back(s.substr(lastPos, pos - lastPos));
    lastPos = s.find_first_not_of(delimiters, pos);
    pos = s.find_first_of(delimiters, lastPos);
  }
}

std::vector<std::string> StringUtil::Tokenize(const std::string& s,
                                              const std::string& delimiters) {
  std::vector<std::string> tokens;
  Tokenize(s, delimiters, tokens);
  return tokens;
}

Fleet::Fleet(int owner,
             int source_planet,
             int destination_planet,
             int num_ships,
             // int total_trip_length,
             // int turns_remaining,
             int launch) {
  owner_ = owner;
  num_ships_ = num_ships;
  source_planet_ = source_planet;
  destination_planet_ = destination_planet;
  // total_trip_length_ = total_trip_length;
  // turns_remaining_ = turns_remaining;
  launch_ = launch;
}

int Fleet::Owner() const {
  return owner_;
}

int Fleet::NumShips() const {
  return num_ships_;
}

int Fleet::SourcePlanet() const {
  return source_planet_;
}

int Fleet::DestinationPlanet() const {
  return destination_planet_;
}

// int Fleet::TotalTripLength() const {
//   return total_trip_length_;
// }

// int Fleet::TurnsRemaining() const {
//   return turns_remaining_;
// }

int Fleet::Launch()const{
  return launch_;
}

int Fleet::Remaining() const{
  return distance_[source_planet_][destination_planet_] + launch_;
}

int Fleet::Length() const{
  return distance_[source_planet_][destination_planet_];
}

Planet::Planet()
{
  Planet(-1,
         -1,
         0,
         0,
         0,
         0,
         true);
}
Planet::Planet(int planet_id,
               int owner,
               int num_ships,
               int growth_rate,
               double x,
               double y,
               bool prediction=true):incoming_fleets_(1,FleetSummary()),
               locked_ships_(0) {
  planet_id_ = planet_id;
  owner_ = owner;
  num_ships_ = num_ships;
  growth_rate_ = growth_rate;
  x_ = x;
  y_ = y;
  update_prediction_ = prediction;
}

int Planet::PlanetID() const {
  return planet_id_;
}

int Planet::Owner() const {
  return owner_;
}

int Planet::Ships(bool locked /* = false */) const {
    return locked ? std::max(num_ships_ - locked_ships_,0) : num_ships_;
}

int Planet::NumShips() const {
  return num_ships_;
}

int Planet::GrowthRate() const {
  return growth_rate_;
}

double Planet::X() const {
  return x_;
}

double Planet::Y() const {
  return y_;
}

void Planet::Owner(int new_owner) {
  UpdatePrediction();
  owner_ = new_owner;
}

void Planet::NumShips(int new_num_ships) {
  UpdatePrediction();
  num_ships_ = new_num_ships;
  UpdatePrediction();
}

void Planet::AddShips(int amount) {
  UpdatePrediction();
  num_ships_ += amount;
  UpdatePrediction();
}

int Planet::RemoveShips(int amount) {
  UpdatePrediction();
    num_ships_ -= amount;
    if ( num_ships_ < 0 ) {
        amount += num_ships_;
        num_ships_ = 0;
    }
    update_prediction_ = true;
    //pwlog<<"after removing ships, the new number of ships are "<<num_ships_<<endl;
  UpdatePrediction();
    return amount;
}

void Planet::AddIncomingFleet(Fleet f) 
{
  UpdatePrediction();
    unsigned int arrival = f.Remaining();
    // ensure incoming_fleets_ is long enough
    if ( arrival + 1 > incoming_fleets_.size() ) {
        incoming_fleets_.resize( arrival + 1, FleetSummary() );
    }

    incoming_fleets_[arrival][f.Owner()] += f.NumShips();     //change this to ships afterwards

    update_prediction_ = true;
  UpdatePrediction();
  pwlog<<"fleet size for planet "<<planet_id_<< " is "<<incoming_fleets_.size()<<endl;

}

std::vector<FleetSummary> Planet::IncomingFleets() const
{
  return incoming_fleets_;
}

int Planet::FutureDays()  {
    UpdatePrediction();
    return prediction_.size() - 1;
}

int Planet::FutureOwner()  {
    UpdatePrediction();
    // for(int i=0;i<incoming_fleets_.size();++i)
    // {
    //   pwlog << i << " , " << incoming_fleets_[i][1] << " , " << incoming_fleets_[i][2] << " ";
    // }
    // pwlog << endl;
    //////pwlog<<"calling future owner and the size of prediction_ array is "<<prediction_.size()<<endl;
    return prediction_.back().owner;
}

PlanetState Planet::FutureState(unsigned int days)  {
    UpdatePrediction();
    if ( days < prediction_.size() ) {
        return prediction_[days];     
    }
    else {
        PlanetState state = prediction_.back();
        if ( state.owner ) {
            state.ships += growth_rate_ * ( days - (prediction_.size() - 1) );
        }
        return state;
    }
}


void Planet::UpdatePrediction() {
    // If fleets have not been updated we don't need to do anything
    if ( not update_prediction_ ) {
        return;
    }
    pwlog<<this->PlanetID()<<endl;
     for(int i=0;i<incoming_fleets_.size();++i)
    {
      pwlog << i << " , " << incoming_fleets_[i][1] << " , " << incoming_fleets_[i][2] << " ";
    }
    pwlog<<endl;
    //////pwlog<<"calling the update prediction function"<<endl;
    prediction_.clear();
    //////pwlog<<"step 1"<<endl;
    // initialise current day
    PlanetState state;
    //////pwlog<<"step 2"<<endl;
    state.owner = owner_;
    //////pwlog<<"step 3"<<endl;
    state.ships = num_ships_ + incoming_fleets_[0][owner_];
    //////pwlog<<"step 4"<<endl;
    prediction_.push_back(state);
    //////pwlog<<"step 5"<<endl;
    std::vector<int> sort_states(3,0);

    //////pwlog<<"size of prediction array is "<<prediction_.size()<<endl;
    //////pwlog<<"size of incoming fleet is "<<incoming_fleets_.size()<<endl;
    for ( unsigned int day=1; day < incoming_fleets_.size(); ++day ) 
    {
        const FleetSummary &f = incoming_fleets_[day];

        // grow planets which have an owner
        if ( state.owner != 0 ) 
        {
            state.ships += growth_rate_;
        }

        if ( ! f.empty() ) 
        {
            // There are ships: FIGHT

            if ( state.owner != 0 ) 
            {
                // occupied planet
                state.ships += f.delta(state.owner);
                if ( state.ships < 0 ) 
                {
                    state.owner = flip(state.owner);
                    state.ships = -state.ships;
                }
              }
            else 
            {
                // neutral planet
                sort_states[0] = state.ships;
                sort_states[1] = f(1);
                sort_states[2] = f(2);
                sort( sort_states.begin(), sort_states.end() );

//highly doubtful
                // the number of ships left is (the maximum minus the second highest)
                state.ships = sort_states[2] - sort_states[1];
                if ( state.ships > 0 ) 
                {
                    // if there was a winner, determine who it was
                    if ( sort_states[2] == f.ally_ ) 
                    {
                        state.owner = 1;
                    }
                    else if ( sort_states[2] == f.enemy_ ) 
                    {
                        state.owner = 2;
                    }
                }
            }
        }

        // update the state
        prediction_.push_back( state );
    }
    //////pwlog<<"size of prediction array at the end of update function is "<<prediction_.size()<<endl;
    //////pwlog<<"exiting from the function"<<endl;
    update_prediction_ = false;
}

int Planet::Cost(unsigned int days, int attacker) 
{
  UpdatePrediction();
   int defender = flip(attacker);

    if ( days >= prediction_.size() ) {
        PlanetState state = FutureState(days);
        return state.owner == attacker ? 0 : state.ships + 1;
    }

    int ships_delta = 0;
    int required_ships = 0;
    int prev_owner = days == 0 ? owner_ : prediction_[days-1].owner;

    if ( prediction_[days].owner == attacker )  {
        ships_delta = prediction_[days].ships;
    }
    else if ( prediction_[days].owner == 0 ) {
        // before anything else we need to ships to overcome the existing force
        required_ships = prediction_[days].ships + 1;

        const FleetSummary &f = incoming_fleets_[days];
        if ( f(attacker) < f(defender) ) {
            // If there is another attacking force (larger than us)
            // we need to overcome that one too
            required_ships += f.delta(defender);
        }
    }
    else if ( prev_owner == 0 ) {
        // opponent just took neutral with this move
        const FleetSummary &f = incoming_fleets_[days];
        required_ships = f.delta(defender) + 1;
    }
    else {
        required_ships = prediction_[days].ships + 1;
    }

    // TODO: Merge with required ships
    for ( unsigned int i = days+1; i < incoming_fleets_.size(); ++i ) {
        const FleetSummary &f = incoming_fleets_[i];
        ships_delta += growth_rate_ + f.delta(attacker);
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

  UpdatePrediction();
    return required_ships;
}

int Planet::RequiredShips() const {

    int ships_delta = 0;
    int required_ships = 0;

    for ( unsigned int day=1; day < incoming_fleets_.size(); ++day ) {
        const FleetSummary &f = incoming_fleets_[day];
        ships_delta += growth_rate_ + f.delta(owner_);
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

    return required_ships;
}

// number of extra ships available to the current owner of this planet
int Planet::ShipExcess(unsigned int days) const {

    int ships_delta = 0;
    int required_ships = 0;

    unsigned int day = 1;
    for ( ; day < incoming_fleets_.size() && day < days; ++day ) {
        const FleetSummary &f = incoming_fleets_[day];
        ships_delta += growth_rate_ + f.delta(owner_);
        if ( ships_delta < 0 ) {
            required_ships += -ships_delta;
            ships_delta = 0; 
        }
    }

    for ( ; day < days; ++day ) {
        ships_delta += growth_rate_;
    }

    return ships_delta;
}
int Planet::LockShips(int ships) {
  UpdatePrediction();
    locked_ships_ += ships;
    //pwlog<< " Locked on " << planet_id_ << ": " << locked_ships_<<endl;
    // LOG( " Locked on " << id << ": " << locked_ships_);
    if ( locked_ships_ > num_ships_ ) {
        ships -= locked_ships_ - num_ships_;
        locked_ships_ = num_ships_;
    }
    return ships;
}

int Planet::LockedShips() {
      return locked_ships_ ;
}

void Planet::UnLockShips(int ships) {
  UpdatePrediction();
    locked_ships_ -= ships;
    //pwlog<< " Locked on " << planet_id_ << ": " << locked_ships_<<endl;
    // LOG( " Locked on " << id << ": " << locked_ships_);
    if ( locked_ships_ <0 ) {
        locked_ships_ = 0;
    }
    // return ships;
}

PlanetWars::PlanetWars(const std::string& gameState) {
  ParseGameState(gameState);
}

int PlanetWars::NumPlanets() const {
  return planets_.size();
}

Planet& PlanetWars::GetPlanet(int planet_id) {
  return planets_[planet_id];
}
const Planet& PlanetWars::GetPlanet(int planet_id) const{
  return planets_[planet_id];
}

int PlanetWars::NumFleets() const {
  return fleets_.size();
}

const Fleet& PlanetWars::GetFleet(int fleet_id) const {
  return fleets_[fleet_id];
}

std::vector<Planet> PlanetWars::Planets() const {
  std::vector<Planet> r;
  for (int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    r.push_back(p);
  }
  return r;
}

std::vector<Planet> PlanetWars::MyPlanets() const {
  std::vector<Planet> r;
  for (int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.Owner() == 1) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Planet> PlanetWars::NeutralPlanets() const {
  std::vector<Planet> r;
  for (int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.Owner() == 0) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Planet> PlanetWars::EnemyPlanets() const {
  std::vector<Planet> r;
  for (int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.Owner() > 1) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Planet> PlanetWars::NotMyPlanets() const {
  std::vector<Planet> r;
  for (int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.Owner() != 1) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Fleet> PlanetWars::Fleets() const {
  std::vector<Fleet> r;
  for (int i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    r.push_back(f);
  }
  return r;
}

std::vector<Fleet> PlanetWars::MyFleets() const {
  std::vector<Fleet> r;
  for (int i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    if (f.Owner() == 1) {
      r.push_back(f);
    }
  }
  return r;
}

std::vector<Fleet> PlanetWars::EnemyFleets() const {
  std::vector<Fleet> r;
  for (int i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    if (f.Owner() > 1) {
      r.push_back(f);
    }
  }
  return r;
}

std::string PlanetWars::ToString() const {
  std::stringstream s;
  for (unsigned int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    s << "P " << p.X() << " " << p.Y() << " " << p.Owner()
      << " " << p.NumShips() << " " << p.GrowthRate() << std::endl;
  }
  for (unsigned int i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    s << "F " << f.Owner() << " " << f.SourcePlanet() << " " << 
    f.DestinationPlanet() << " " << f.NumShips() << " "  
      << f.launch_ << std::endl;
  }
  return s.str();
}

int PlanetWars::Distance(int source_planet, int destination_planet) const {
  const Planet& source = planets_[source_planet];
  const Planet& destination = planets_[destination_planet];
  double dx = source.X() - destination.X();
  double dy = source.Y() - destination.Y();
  return (int)ceil(sqrt(dx * dx + dy * dy));
}

std::vector<Fleet> PlanetWars::Orders() const {
    return orders_;
}

// void PlanetWars::PlaceOrder(const Fleet& order){

// }
  
// PlanetWars::Orders()
// {
//     return orders_;
// }
void PlanetWars::AddFleet(const Fleet& order) {
    if ( order.launch_ >= 0 ) {
        planets_[order.SourcePlanet()].RemoveShips( order.NumShips() );
    }
    //pwlog<<"dest planet:"<<order.DestinationPlanet()<<endl;
    planets_[order.DestinationPlanet()].AddIncomingFleet( order );
}

void PlanetWars::PlaceOrder(const Fleet& order) {
    AddFleet(order);

    if ( order.Owner() != 1 ) return;

    // only store order if it is for now
    if ( order.launch_ == 0 ) {
        orders_.push_back(order);
    }
    // else {
    //     LOG( "  DELAYED ORDER: " << order.source << " " << order.dest << " " << order.ships << " | " << order.launch );
    // }
}



void PlanetWars::IssueOrder(int source_planet,
                            int destination_planet,
                            int num_ships) const {
  std::cout << source_planet << " "
            << destination_planet << " "
            << num_ships << std::endl;
  std::cout.flush();
}

bool PlanetWars::IsAlive(int player_id) const {
  for (unsigned int i = 0; i < planets_.size(); ++i) {
    if (planets_[i].Owner() == player_id) {
      return true;
    }
  }
  for (unsigned int i = 0; i < fleets_.size(); ++i) {
    if (fleets_[i].Owner() == player_id) {
      return true;
    }
  }
  return false;
}

int PlanetWars::NumShips(int player_id) const {
  int num_ships = 0;
  for (unsigned int i = 0; i < planets_.size(); ++i) {
    if (planets_[i].Owner() == player_id) {
      num_ships += planets_[i].NumShips();
    }
  }
  for (unsigned int i = 0; i < fleets_.size(); ++i) {
    if (fleets_[i].Owner() == player_id) {
      num_ships += fleets_[i].NumShips();
    }
  }
  return num_ships;
}

int PlanetWars::Production(int player_id) const{
  int production = 0;
  for(unsigned int i=0; i<planets_.size();++i)
  {
    if(planets_[i].Owner() == player_id){
      production += planets_[i].GrowthRate();
    }
  }
  return production;
}

int PlanetWars::ParseGameState(const std::string& s) {
  planets_.clear();
  fleets_.clear();
  std::vector<std::string> lines = StringUtil::Tokenize(s, "\n");
  int planet_id = 0;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    std::string& line = lines[i];
    size_t comment_begin = line.find_first_of('#');
    if (comment_begin != std::string::npos) {
      line = line.substr(0, comment_begin);
    }
    std::vector<std::string> tokens = StringUtil::Tokenize(line);
    if (tokens.size() == 0) {
      continue;
    }
    if (tokens[0] == "P") {
      if (tokens.size() != 6) {
        return 0;
      }
      Planet p(planet_id++,              // The ID of this planet
	       atoi(tokens[3].c_str()),  // Owner
               atoi(tokens[4].c_str()),  // Num ships
               atoi(tokens[5].c_str()),  // Growth rate
               atof(tokens[1].c_str()),  // X
               atof(tokens[2].c_str())); // Y
      planets_.push_back(p);
    } else if (tokens[0] == "F") {
      if (tokens.size() != 7) {
        return 0;
      }
      Fleet f(atoi(tokens[1].c_str()),  // Owner
              atoi(tokens[3].c_str()),  // Source
              atoi(tokens[4].c_str()),  // Destination
              atoi(tokens[2].c_str()),  // Num ships
              -atoi(tokens[5].c_str())+atoi(tokens[6].c_str())); // Turns remaining-total length
      fleets_.push_back(f);
    } else {
      return 0;
    }
   
  }
   for(int i=0;i<fleets_.size();++i)
    {
      pwlog<<"Bitch "<<fleets_[i].Owner()<<", "<<fleets_[i].NumShips()<<" , "<<fleets_[i].SourcePlanet()<<" , "<<fleets_[i].DestinationPlanet()<<" "<<endl;;
      Fleet f = fleets_[i];
    //   planets_[f.DestinationPlanet()].AddIncomingFleet(f);
      AddFleet(f);
    }
    pwlog<<endl;
  return 1;
}

void PlanetWars::FinishTurn() const {
  std::cout << "go" << std::endl;
  std::cout.flush();
}

struct SortPlanets
{
  std::vector<int>& i;
  SortPlanets(std::vector<int>& a):i(a){}
  bool operator()(int a, int b)const
  {
    return i[a]<i[b];
  }
};

void PlanetWars::Init()
{
  pwlog.open("pwlog.txt", std::ios::out);
  // //////pwlog<<"starting initialisation"<<std::endl;
  distance_.resize(planets_.size());
  // //////pwlog<<"resizing done"<<std::endl;
  // //////pwlog<<"the number of planets are"<<planets_.size()<<std::endl;
  for(int i=0;i<planets_.size();++i)
  {
    distance_[i].resize(planets_.size());
  }
  for(int i=0;i<planets_.size();++i)
  {
    // //////pwlog<<"starting filling for planet "<<i<<std::endl;
    int planeta = planets_[i].PlanetID();
    for(int j=i;j<planets_.size();++j)
    {
      int planetb = planets_[j].PlanetID();
      int distance = Distance(planets_[i].PlanetID() , planets_[j].PlanetID());
      // //////pwlog<<"distance called for pair "<<i<<" "<<j<<" and the distance is "<<distance<<std::endl;
      distance_[planeta][planetb] = distance;
      distance_[planetb][planeta] = distance;
    }
  }
   // for(int i=0;i<distance_.size();++i)
   //  {
   //     for(int j=0;j<distance_.size();++j)
   //    {
   //        //////pwlog<<distance_[i][j]<<" ";
   //    }
   //    //////pwlog<<std::endl;
   //  }
   //  //////pwlog<<std::endl;
  // //////pwlog<<"distance vector filled"<<std::endl;

  by_distance_.resize(planets_.size());
  for(int i=0;i<planets_.size();++i)
  {
    // //////pwlog<<"process for "<<i<<" started "<<std::endl;
    by_distance_[i].resize(planets_.size());
    // //////pwlog<<"restting size for "<<i<<" done "<<std::endl;
    for(int j=0;j<planets_.size();++j)
    {
      by_distance_[i][j] = j;
     // //////pwlog<<j<<" inserted into vector for "<<i<<std::endl;
    }
    // //////pwlog<<"sorting for distance vector for array "<<i<<" started "<<std::endl;
    sort(by_distance_[i].begin(), by_distance_[i].end(), SortPlanets(distance_[i]));
    // //////pwlog<<"sorting for distance vector for array "<<i<<" done "<<std::endl;
    by_distance_[i].erase(by_distance_[i].begin());
  }

   // for(int i=0;i<by_distance_.size();++i)
   //  {
   //     for(int j=0;j<by_distance_.size();++j)
   //    {
   //        //////pwlog<<by_distance_[i][j]<<" ";
   //    }
   //    //////pwlog<<std::endl;
   //  }
   //  //////pwlog<<std::endl;

  // //////pwlog<<"initialisation complete"<<std::endl;
}

int PlanetWars::ClosestPlanetByPlayer(int planet , int player) const
  {
    //////pwlog<<"calling ClosestPlanetByPlayer with arguements "<<planet<<" , "<<player<<std::endl;
    std::vector<int> planets_sorted(PlanetsByDistance(planet));
    //////pwlog<<"size of planets_sortd is "<<planets_sorted.size()<<std::endl;
    for(int i=0;i<planets_sorted.size();++i)
    {
      //////pwlog<<planets_sorted[i]<<" ";
    }
    //////pwlog<<std::endl;
    for(int i=0;i<planets_sorted.size();++i)
    {
      // //////pwlog<<"Owner of "<<planets_sorted[i]<<" is "<<
      if(GetPlanet(planets_sorted[i]).Owner() == player)
       {//////pwlog<<"done calling ClosestPlanetByPlayer with arguements "<<planet<<" , "<<player<<std::endl;
        return planets_sorted[i];}
    }
    return -1;

  }

std::vector<int> PlanetWars::PlanetsOwnedBy(int player) const
{
  std::vector<int> result;
  for(int i=0;i<planets_.size();++i)
  {
    if(planets_[i].Owner()==player)
    {
      result.push_back(planets_[i].PlanetID());
    }
  }
  return result;
}

std::vector<int> PlanetWars::PlanetsByDistance(int planet_id) const
{
  //////pwlog<<"Calling the vector return function"<<std::endl;
  // //////pwlog<<by_distance_[planet_id].size()<<std::endl;
  return std::vector<int>(by_distance_[planet_id]);
}

int PlanetWars::ShipsWithinRange(int planet, int distance, int owner) const {
    const std::vector<int>& sorted = PlanetsByDistance(planet);

    int ships = 0;

    for(int i=0;i<sorted.size();++i){
        int helper_distance = Distance(planet, sorted[i]);
        if ( helper_distance >= distance ) break;

        if ( planets_[sorted[i]].Owner() != owner ) continue;
        ships += planets_[sorted[i]].Ships();         //change this to ships afterwars, using the locked concept
    }
    
    return ships;
}

// Find planets closest to the opponent
std::map<int,bool> PlanetWars::FrontierPlanets(int player) const {
    // TODO: use bitmap
    std::map<int,bool> frontier_planets;
    std::vector<int> opponent_planets = PlanetsOwnedBy(flip(player));
    //////pwlog<<"The following are the opponent planets: "<<" (#"<<opponent_planets.size()<<" )"<<std::endl;
    // for(int i=0;i<opponent_planets.size();++i)
    // {
    //   //////pwlog<<opponent_planets[i]<<" ";
    // }
    //////pwlog<<endl;
    for(int i=0;i<opponent_planets.size();++i){
        Planet p = GetPlanet(opponent_planets[i]);
        int closest = ClosestPlanetByPlayer(p.PlanetID(),player);
        if ( closest != -1 ) {
            frontier_planets[closest] = true;
        }
    }
    return frontier_planets;
}

std::vector<int> PlanetWars::FrontierPlanetsVector(int player) const {
    // TODO: use bitmap
    std::map<int,bool> frontier = FrontierPlanets(player);
    vector<int> frontier_planets;
    for(map<int, bool> :: iterator it = frontier.begin() ; it!= frontier.end();++it)
    {
      if(it->second){frontier_planets.push_back(it->first);}
    }
    return frontier_planets;
}

std::vector<int> PlanetWars::NotFrontierPlanets(int player) const{
    std::map<int,bool> frontier = FrontierPlanets(player);
    vector<int> not_frontier_planets;
    vector<Planet> my_planets = MyPlanets();
    for(int i=0;i<my_planets.size();++i)
    {
      if(frontier.find(my_planets[i].PlanetID()) == frontier.end()){
        not_frontier_planets.push_back(my_planets[i].PlanetID());
      }
    }
    return not_frontier_planets;
}

double PlanetWars::Potential(int planet_id, int player)
{
    double pot = 0;
    vector<Planet> enemy_  = EnemyPlanets();
    for(int i=0;i<enemy_.size();++i)
    {
      pot += (enemy_[i].NumShips()*1.0)/(Distance(planet_id , enemy_[i].PlanetID()));
    }
    return pot;
}

// Find planets that will be closest to the opponent
std::map<int,bool> PlanetWars::FutureFrontierPlanets(int player) {
  //////pwlog<<"trying to find future frontier planets of player "<<player<<endl;
    std::map<int,bool> frontier_planets;

    std::vector<int> opponent_planets = PlanetsOwnedBy(flip(player));         // i think this should be player and not flip player, similar for frontier planets
    //////pwlog<<"size of opponent_planets is "<<opponent_planets.size()<<endl;

    for(int i=0;i<opponent_planets.size();++i)
    {
      int opponent_planet = opponent_planets[i];
        const std::vector<int>& sorted = PlanetsByDistance( opponent_planet );
        //////pwlog<<"the size of array containing planets sorted as distance fron the given planet is "<<sorted.size()<<endl;
        int closest=-1;
        for(int j=0;j<sorted.size();++j) 
        {
            int p = sorted[j];
            if ( planets_[p].FutureOwner() == player ) 
            {
                closest = planets_[p].PlanetID();
                break;
            }
        }
        //////pwlog<<"end of for loop"<<endl;
        if ( closest!= -1 ) 
        {
            frontier_planets[closest] = true;
        }
    //////pwlog<<"finished checking planet "<<opponent_planet<<endl;
    }
    //////pwlog<<"finished calling the function FutureFrontierPlanets "<<endl;
    return frontier_planets;
}