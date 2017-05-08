
#include <iostream>
#include <cmath>
#include <map>
#include <fstream>
#include <set>
#include <algorithm>
#include "PlanetWars.h"


using namespace std;

#define INF 9999999
ofstream file;

int maxGrowthRate = 0 ;

typedef std::map<int, std::pair<int,int> > DefenceExclusions;

int turn_number;

map<int, bool> my_half;

bool FillOrder(PlanetWars& pw, int dest_id, std::vector<Fleet>& orders, int player, int day, int& cost)
{
    vector<int> planets_by_dist = pw.PlanetsByDistance(dest_id);
    vector<int> my_planets_by_dist;
    for(int i=0;i<planets_by_dist.size();++i)
    {
        if(pw.Distance(planets_by_dist[i], dest_id) > day)break;
        if(pw.GetPlanet(planets_by_dist[i]).Owner() == player)
        {
            my_planets_by_dist.push_back(planets_by_dist[i]);
        }
    }
    Planet& dest =  pw.GetPlanet(dest_id);
    int y = dest.FutureState(day).ships + 1;
    bool success = true;
    cost = y;
    for(int i=0;i<my_planets_by_dist.size();++i)
    {
        if(y==0) break;
        int delay = day - pw.Distance(my_planets_by_dist[i],dest_id);
        int can_send = min(y, pw.GetPlanet(my_planets_by_dist[i]).Ships(true));
        y -= max(0,can_send);
       orders.push_back(Fleet(player, my_planets_by_dist[i],dest_id,can_send,delay)); 
    }
    if(y>0)
    {
        success = false;
        orders.clear();
    }
    file << "FillOrder "<<dest_id<<" cost is "<<cost<<" on day "<<day<<" is a success ? "<<success<<" y remaining"<<y<<endl;
    return success;
}

int ScoreEdge(PlanetWars& pw, int dest_id, int source_id, int available_ships, int source_ships, int& cost, std::vector<Fleet>& orders, int player) {
    // static double distance_scale = Config::Value<double>("cost.distance_scale");
    // static double growth_scale   = Config::Value<double>("cost.growth_scale");
    // static int    cost_offset    = Config::Value<int>("cost.offset");

    // int source_id = source->id;
    // int dest_id = dest->id;

    Planet& dest = pw.GetPlanet(dest_id);
    Planet& source = pw.GetPlanet(source_id);

    int future_days = dest.FutureDays();
    int future_owner = dest.FutureOwner();
    int growth_rate = dest.GrowthRate();

    int distance = pw.Distance( dest_id, source_id );
    cost = INF;
    int score = INF;
    int extra_delay = 0;

    int ships_within_range = pw.ShipsWithinRange(dest_id, distance, flip(player));
    if(future_owner == player) return score;
    if ( distance > future_days ) {
        // easy case: we arrive after all the other fleets
        PlanetState prediction = dest.FutureState( distance );
        cost = prediction.ships;


        if ( future_owner ) {

                int score_cost = dest.Cost(distance,player)+1;
                // int score_cost = cost + ships_within_range;
                // if ( dest.Owner() == flip(player) ) cost = score_cost;

                // // TODO: determine the best factor for distance
                score = ceil((double)score_cost/growth_rate);       //check if the constants are okay
                // score = ceil((double)score_cost/growth_rate + distance);       //check if the constants are okay

            // score = distance + distance/2;

                // if(future_owner == player) cost = 0; 
            // LOG( "  to attack " << dest_id << " from " << source_id << ": cost = " << cost << ", score_cost = " << score_cost << ", score = " << score );
        }
        else {
            // For a neutral planet:
            //   the number of days to travel to the planet
            //   + time to regain units spent
            int score_cost = cost;// + ships_within_range;
            score = ceil((double)score_cost/growth_rate);      //made a modification,why should n't constants be used here
            // score = ceil((double)score_cost/growth_rate) + distance;      //made a modification,why should n't constants be used here
        }
    }
    else {
        // hard case: we can arrive before some other ships
        // we know that this planet is (or will be) eventually be owned 
        // by an enemy
        int best_score = INF;
        int best_cost = 0;

        // determine the best day to arrive on, we search up to 1 day AFTER the last fleet arrives
        for ( int arrive = future_days+1; arrive >= distance; --arrive ) {
            // TODO: Another magic param 
            int cost = dest.Cost( arrive, player )+1; 
            // int cost = dest.Cost( arrive, player ); 
            int score_cost = cost;// + ships_within_range;
            if ( dest.Owner() == flip(player) ) cost = score_cost;

            // int score = arrive + arrive/2;
            int score = (int)((double)score_cost/growth_rate);
            // int score = (int)((double)score_cost/growth_rate + arrive);
            if ( score < best_score ) {
                best_score = score;
                extra_delay = arrive - distance;
                best_cost = cost;
            }
        }

        score = best_score;
        cost = best_cost;
    }

    cost += 1;
    if ( cost < 0 ) {
        cost = 0;
    }

    int required_ships = 0;
    if ( cost > available_ships ) {
        required_ships = source_ships;
    }
    else {
        required_ships = source_ships - ( available_ships - cost );
    }

    if ( required_ships < 0 ) {
        required_ships = 0;
        // Fix the WTF
        // LOG_ERROR( "WTF: attacking " << dest_id << ": " << cost << " " << available_ships << " " << source_ships );
    }
    // int nearest_frontier = pw.ClosestPlanetByPlayer(dest_id , player);
    // if(nearest_frontier != source_id)
    // {
    //     int turns_to_reach_frontier = pw.Distance(source_id, nearest_frontier);
    //     int turns_to_reach_dest = pw.Distance(nearest_frontier , dest_id);
    //     extra_delay = max(0, extra_delay-(turns_to_reach_frontier+turns_to_reach_dest-distance));
    //     dest_id = nearest_frontier;
    // }

    //file<<"pushing order of size "<<required_ships<<" from "<<source_id<<" to "<<dest_id<<" by player "<<player<<" with extra_delay "<<extra_delay<<endl;
    orders.push_back( Fleet(player, source_id, dest_id, required_ships, extra_delay) ); 
    //file<<"the cost of this scoreedge is "<<cost<<endl;
    return score;
}

int AttackPlanet(PlanetWars& pw, int p_id,const DefenceExclusions& defence_exclusions, std::vector<Fleet>& orders, int player) {
    //file<<"entering the attack planet function"<<endl;
    Planet& p = pw.GetPlanet(p_id);
    // sort player planets in order of distance the target planet
    const std::vector<int>& all_sorted =pw.PlanetsByDistance( p_id );
    std::vector<int> my_sorted;
    for(int i =0; i<all_sorted.size();i++) {
      int source_id = all_sorted[i];
        if ( pw.GetPlanet(source_id).Owner() == player ) {
            my_sorted.push_back( source_id );
        }
    }

    // determine distance to closest enemy
    int closest_enemy = pw.ClosestPlanetByPlayer( p_id, flip(player) );
    int closest_enemy_distance = (closest_enemy!=-1) ? pw.Distance(closest_enemy, p_id) : INF;
    int future_owner = p.FutureOwner();

    int score = INF;
    int cost = INF;

    int available_ships = 0;
    for(int i=0;i<my_sorted.size();++i) 
    {
      int source_id = my_sorted[i];
        if ( cost <= available_ships ) break;

        const Planet& source = pw.GetPlanet(source_id);
        int source_ships = source.Ships(true);         //change numships to ships()
        //below part already checked when calling from attack
        // if ( future_owner == 0 ) {
        //     // Don't attack neutral planets closer to the enemy
        //     int distance = pw.Distance( source_id, p_id );
        //     // if ( closest_enemy_distance <= distance && p->FutureDays() < distance ) {
        //     if ( closest_enemy_distance <= distance ) {
        //         // We do not want this move to be considered AT ALL
        //         // So give it the highest score
        //         cost = INF;
        //         score = INF;
        //         break;
        //     }
        // }

        // DefenceExclusions::const_iterator d_it = defence_exclusions.find(source_id);
        // if ( d_it != defence_exclusions.end() && d_it->second.first == p_id ) {
        //     //file<<" Lifting defence exclusion for " << d_it->second.second << " ships on planet " << source_id <<endl;
        //     // LOG( " Lifting defence exclusion for " << d_it->second.second << " ships on planet " << source_id );
        //     source_ships += d_it->second.second;                //  i think this should be minus
        // }

        available_ships += source_ships;

        score = ScoreEdge(pw, p_id, source_id, available_ships, source_ships, cost, orders, player);
        //file<<"the score for this attack is "<<score<<endl;
    }

    // If the cost is too large then we can generate no orders
    if ( cost > available_ships ) {
      //file<<"cost is too high"<<endl;
        orders.clear();
    }

    // if ( orders.size() > 0 ) {
    //     LOG( "  score of planet " << p_id << " = " << score << " (" <<  cost << ") after " << orders.back().launch << " days" );
    // }
    // else {
    //     LOG( "  not enough ships to attack " << p_id );
    // }

    return score;
}

// double EvalState( PlanetWars& state ) { 
//     // int score = 0;
//     vector<Planet> planets = state.Planets();
//     int my_ships = 0, enemy_ships = 0;
//     for(int i=0;i<planets.size();i++){
//         Planet& p =planets[i]; 
//         PlanetState s = p.FutureState(50);
//         s.owner == 1? (my_ships+=s.ships): (enemy_ships+=s.ships);
//     }
//     double score = my_ships - enemy_ships;
//     // double score = double((1+my_ships)*1.0/(1+enemy_ships*1.0) );
//     return score;
// }

bool FullAttack(PlanetWars& pw,int p,int player)
{
    file<<"fullattack called for planet "<<p<<endl;
    PlanetWars dummy = pw;
    vector<Planet> all_planets = dummy.Planets();
    for(int i=0;i<all_planets.size();++i)
    {
        dummy.GetPlanet(i).UnLockShips(all_planets[i].LockedShips());
    }
    Planet& current = dummy.GetPlanet(p);
    for(int i=0;i<all_planets.size();++i)
    {
        Planet& attacking = all_planets[i];
        if(attacking.PlanetID() == p)continue;
        if(attacking.Owner()==0)continue;
        dummy.PlaceOrder(Fleet(attacking.Owner(),attacking.PlanetID(), p, attacking.Ships(),0));
    }
    for(int i=0;i<dummy.GetPlanet(p).FutureDays()+1;++i)
    {
        file<<dummy.GetPlanet(p).FutureState(i).owner<<","<<dummy.GetPlanet(p).FutureState(i).ships<<"     ";
    }
    file<<endl;
    file<<"future_owner is "<<current.FutureOwner()<<endl;
    if(current.FutureOwner()==flip(player)){file<<player<<" will loose, don't attack "<<endl;;return false;}
    file<<"Full attack returns true"<<endl;
    return true;
}

double EvalState( PlanetWars& state)
{
    if(turn_number <0)
    {
        vector<Planet> my_planets = state.MyPlanets();
        int full_attack_score = 0;
        for(int i=0;i<my_planets.size();i++)
        {
            full_attack_score +=  FullAttack(state, my_planets[i].PlanetID(), 1);
        }
        return full_attack_score;
    }
    else
    {
         // int score = 0;
        vector<Planet> planets = state.Planets();
        int my_ships = 0, enemy_ships = 0;
        for(int i=0;i<planets.size();i++){
            Planet& p =planets[i]; 
            PlanetState s = p.FutureState(50);
            s.owner == 1? (my_ships+=s.ships): (enemy_ships+=s.ships);
        }
        double score = my_ships - enemy_ships;
        // double score = double((1+my_ships)*1.0/(1+enemy_ships*1.0) );
        return score;
    }
}

double CombinationAttack(PlanetWars& state, const DefenceExclusions& defence_exclusions,std::vector<int>& targets, int player, unsigned int i=0) {
    if ( i == 0 ) {                                     //this is exponential
        //file<<" Starting attacks (combination)"<<endl;
    }
    //file<<"entering into combination attack for "<<i<<endl;


    if ( i >= targets.size() ) {
       //file<<"exiting from combination attack for "<<i<<endl;
        return EvalState(state);
    }

    PlanetWars attack_state = state;

    double attack_eval = -INF;

    // Try attacking targets[i]
    std::vector<Fleet> orders;
    // file<<"turn("<<i<<"), the attck score is "<<
    AttackPlanet(attack_state, targets[i], defence_exclusions, orders, player);//<<endl;
    //file<<" and the size of orders is "<<orders.size()<<endl;
    if (  orders.size()>0 ) 
    {
        //file<<"entering the if condition"<<endl;
        const Fleet& last_order = orders.back();
        //file<<"step 2"<<endl;
        // update delays so that all fleets arrive at a neutral at the same time
        // if ( attack_state.GetPlanet(last_order.DestinationPlanet()).FutureOwner() == 0 ) {
        //     int last_arrival = last_order.Launch() + state.Distance( last_order.SourcePlanet(), last_order.DestinationPlanet() );
        //     for(int k=0;k<orders.size();++k){
        //         Fleet& order = orders[k];
        //         order.launch_ = last_arrival - state.Distance( order.SourcePlanet(), order.DestinationPlanet() );
        //     }
        // }
        //file<<"step 3"<<endl;

        // If we reached here we want to actually execute the orders
        for(int j=0;j<orders.size();++j) {
           Fleet& order = orders[j];
        //file<<"step 3.1"<<endl;

            attack_state.PlaceOrder(order);
        //file<<"step 3.2"<<endl;
        }
        //file<<"step 4"<<endl;

        attack_eval = CombinationAttack(attack_state, defence_exclusions,targets, player, i+1);
        //file<<"step 5"<<endl;

    }
    //file<<"reached at the end of if conndition for "<<i<<endl;
    // Try not attacking targets[i]
    double eval = CombinationAttack(state, defence_exclusions,targets, player, i+1);

    // LOG( "  " << i << ": attack_state " << attack_eval << ", state " << eval );
    // update with most valuable state
    if ( attack_eval > eval ) {
        state = attack_state;
        eval = attack_eval;
    }
    //file<<"exiting from combination attack for "<<i<<endl;
    return eval;
}


void Attack(PlanetWars& state, DefenceExclusions& defence_exclusions,int player, vector<int>& targets) {
    // static bool attack = Config::Value<bool>("attack");
    // if ( ! attack  ) return;

    // LOG("Attack phase");

    // std::vector<int> targets = FindTargets(state, defence_exclusions, player);
  //file<<"inside the attack function and calling combination attack from here "<<endl;
    // if(targets.size() > 10){file<<"pruning targets"<<endl;;targets.erase(targets.begin()+10, targets.end());}
    CombinationAttack(state, defence_exclusions, targets, player);
}

int AntiRageRequiredShips(const PlanetWars &state, const Planet& my_planet, const Planet& enemy_planet);

// Lock the required number of ships onto planets that are under attack
void Defence(PlanetWars& state, int player) 
{
    std::vector<int> my_planets = state.PlanetsOwnedBy(player);

    // map<int, bool> 
    for(int i=0;i<my_planets.size();++i)
    {
        Planet& p = state.GetPlanet(my_planets[i]);
        int prediction_length = p.FutureDays();
        int required_ships=0;
        int min_ships = 99999999;
        for(int j= prediction_length ; j>=0;j--)
        {
            PlanetState thisState = p.FutureState(j);
            if(thisState.owner == flip(player))
            {
                p.LockShips(p.Ships(true));
            }
            else
            {
                min_ships = min(min_ships, thisState.ships);
            }
        }
        p.LockShips(max(0,p.Ships(true)- min_ships));
        if(p.FutureOwner() == flip(player)){p.LockShips(p.Ships(true));}
        int closest_enemy = state.ClosestPlanetByPlayer(p.PlanetID(), flip(player));
        int extra_required = max(0, state.GetPlanet(closest_enemy).NumShips() - (state.Distance(closest_enemy, p.PlanetID())*p.GrowthRate()) ) ;
        p.LockShips(extra_required/2);
        // if(danger){need_help_planets.push_back(make_pair(my_planets[i], make_pair(status_curr.ships+5,j)));}
    }
    // DefenceExclusions v;

    // for(int i=0;i<my_planets.size();++i)
    // {
    //     Planet& p = state.GetPlanet(my_planets[i]);
    //     int prediction_length = p.FutureDays();
    //     int required_ships=0;
    //     int min_ships = 99999999;
    //     std::vector<Fleet> orders;
    //     for(int j= prediction_length ; j>=0;j--)
    //     {
    //         PlanetState thisState = p.FutureState(j);
    //         if(thisState.owner == flip(player))
    //         {
    //             AttackPlanet(state, my_planets[i], v, orders, player);
    //             break;
    //         }
    //     }
    //     for(int j=0;j<orders.size();++j)
    //         state.PlaceOrder(orders[j]);
    //     }
// bool FillOrder(PlanetWars& pw, int dest_id, std::vector<Fleet>& orders, int player, int day)
    // }
    for(int i=0;i<my_planets.size();++i)
    {
        Planet& p = state.GetPlanet(my_planets[i]);                         //order of defence
        int min_cost = 9999999;
        std::vector<Fleet> final_orders;
        if(p.FutureOwner() == player) continue;
        for(int j=0;j<50;j++)
        {
            vector<Fleet> orders;
            int cost;
            bool success = FillOrder(state ,my_planets[i],orders, player, j, cost);
            if(success)
            {
                if(min_cost>cost)
                {
                    min_cost = cost;
                    final_orders = orders;
                }
            }
        }
        if(final_orders.size()>0)
        {
            for(int j=0;j<final_orders.size();++j)
            {
                state.PlaceOrder(final_orders[j]);
            }
        }
    }

    vector<Fleet> all_fleets = state.MyFleets();
    vector<int> targetings;

    for(int i=0;i<all_fleets.size();++i)
    {
        Planet& dest = state.GetPlanet(all_fleets[i].DestinationPlanet());
        if(dest.Owner() == player )
        {
            continue;
        }
        targetings.push_back(dest.PlanetID());
    }
    for(int i=0;i<targetings.size();++i)
    {
        file<<"bitch "<<targetings[i]<<" ";
    }
    file<<endl;
    sort( targetings.begin(), targetings.end() );
    targetings.erase( unique( targetings.begin(), targetings.end() ), targetings.end() );

    for(int i=0;i<targetings.size();++i)
    {
        file<<"bitch returns "<<targetings[i]<<" ";
    }
    file<<endl;
    for(int i=0;i<targetings.size();++i)
    {
        Planet& dest = state.GetPlanet(targetings[i]);
        // if(dest.Owner() == player )
        // {
        //     continue;
        // }
        if(dest.FutureOwner() == player)
        {
            file<<"Future Owner Bitch !!! "<<targetings[i]<<endl;
            continue;
        }
        
        int min_cost = 9999999;
        std::vector<Fleet> final_orders;
        for(int j=0;j<50;j++)
        {
            vector<Fleet> orders;
            int cost;
            bool success = FillOrder(state ,dest.PlanetID(),orders, player, j, cost);
            if(success)
            {
                if(min_cost>cost)
                {
                    min_cost = cost;
                    final_orders = orders;
                }
            }
        }
        if(final_orders.size()>0)
        {
            for(int j=0;j<final_orders.size();++j)
            {
                state.PlaceOrder(final_orders[j]);
            }
        }
    }


    // file<<"entering reinforce function"<<endl;
    // std::vector<Planet> neutral_planets = pw.NeutralPlanets();
    // DefenceExclusions v;
    // for(int i=0;i<neutral_planets.size();++i)
    // {
    //     file<<" "<<"Planet : "<<neutral_planets[i].PlanetID()<<endl;
    //     std::vector<Fleet> orders;
    //     std::vector<FleetSummary> temp = neutral_planets[i].IncomingFleets();
    //     if(neutral_planets[i].FutureOwner() == flip(player))
    //     {
    //         file<<" "<<"finally in enemy's hand"<<endl;
    //         for(int j=0;j<temp.size();++j)
    //         {
    //             file<<" "<<temp[j][player]<<" ";
    //             if(temp[j][player] > 0)
    //             {
    //                 AttackPlanet(pw, neutral_planets[i].PlanetID(), v, orders, player);
    //                 file<<"\n  size of orders is"<<orders.size()<<endl;
    //                 break;
    //             }
    //         }
    //         for(int j=0;j<orders.size();++j)
    //         {
    //             file<<"sending "<<orders[j].NumShips()<<" from "<<orders[j].SourcePlanet()<<" to "<<orders[j].DestinationPlanet()<<endl;
    //             pw.PlaceOrder(orders[j]);
    //         }
    //     }
    //     else
    //     {
    //         file<<" "<<"safe"<<endl;
    //     }
    // }
}

DefenceExclusions AntiRage(PlanetWars& state, int player) {
    DefenceExclusions defence_exclusions; 

    // Anti rge level
    // 0: None    |
    // 1: closest | Increasing number of ships locked
    // 2: max     |
    // 3: sum     v
    // static int anti_rage_level = Config::Value<int>("antirage");
    // static bool have_defence_exclusions = Config::Value<bool>("antirage.exlusions");
    int anti_rage_level = 0 ;
    bool have_defence_exclusions = 1;
    if ( anti_rage_level == 0 ) return defence_exclusions;

    // LOG("Antirage phase");
    //file<<"Antirage phase"<<endl;

    std::map<int,bool> frontier_planets = state.FrontierPlanets(player);
    std::pair<int,bool> item;
    // foreach ( item, frontier_planets ) {
    for(map<int, bool>:: iterator it = frontier_planets.begin();it!= frontier_planets.end();++it){
      pair<int , bool > item = *it;
        if ( ! item.second ) continue;

        Planet p = state.GetPlanet(item.first);

        int ships_locked = 0;

        // the planet we are protecting ourselves from
        Planet rage_planet;
        if(anti_rage_level != 0)
        {
        if ( anti_rage_level == 1 ) {
            // defend against only the closest enemy
            int closest_enemy = state.ClosestPlanetByPlayer(p.PlanetID(), flip(player));
            if ( closest_enemy !=-1 ) {
                ships_locked = AntiRageRequiredShips(state, p, state.GetPlanet(closest_enemy) );
                rage_planet = state.GetPlanet(closest_enemy);
            }
        }
        else {
            // defend against all enemies
            int max_required_ships = 0;
            int sum_required_ships = 0;
            std::vector<int> enemy_planets = state.PlanetsOwnedBy(flip(player));
            // foreach ( PlanetPtr& enemy_planet, enemy_planets ) {
            for(int i=0;i<enemy_planets.size();++i){
                Planet& enemy_planet = state.GetPlanet(enemy_planets[i]);
                int required_ships = AntiRageRequiredShips(state, p, enemy_planet );
                if ( required_ships > max_required_ships ) {
                    max_required_ships = required_ships;
                    rage_planet = enemy_planet;
                }
                sum_required_ships += required_ships;
            }

            ships_locked = anti_rage_level == 2 ? max_required_ships : sum_required_ships;
        }

        if ( ships_locked > 0 ) {
            ships_locked = p.LockShips(ships_locked);
            if (  have_defence_exclusions ) {
                defence_exclusions[p.PlanetID()] = std::pair<int,int>(rage_planet.PlanetID(), ships_locked);
            }
            // LOG( " Locking " << ships_locked << " ships on planet " << p->id );
            //file<<" Locking " << ships_locked << " ships on planet " << p.PlanetID()<<endl;
        }
    }

    return defence_exclusions;
}
}

int AntiRageRequiredShips(const PlanetWars &state, const Planet& my_planet, const Planet& enemy_planet) {
    int distance = state.Distance(enemy_planet.PlanetID(), my_planet.PlanetID());
    int required_ships = enemy_planet.Ships() - distance*my_planet.GrowthRate();         // change numships to ships
    if ( required_ships <= 0 ) return 0;

    int player = my_planet.Owner();

    // enslist help
    const std::vector<int>& sorted = state.PlanetsByDistance( my_planet.PlanetID() );
    // foreach ( int i, sorted ) {
    for(int j=0;j<sorted.size();++j) {
      int i= sorted[j];
        const Planet p = state.GetPlanet(i);
        if ( p.Owner() != player ) continue;

        int help_distance =  state.Distance(my_planet.PlanetID(), i);
        if ( help_distance >= distance ) break;
        required_ships -= p.Ships(true) + p.ShipExcess(distance-help_distance-1);         //change numships to ships(true)
    }

    if ( required_ships <= 0 ) return 0;

    return required_ships;
}


vector<int> FindTargets(PlanetWars& pw,int player, DefenceExclusions& defence_exclusions)
{
    vector<Planet> not_my_planets = pw.NotMyPlanets();
    std::map<int,bool> enemy_frontier = pw.FrontierPlanets(flip(player));               //check if these are properly calculated
    std::map<int,bool> enemy_frontier_future = pw.FutureFrontierPlanets(flip(player));
  vector<pair<int,int > > destination_planets;
    bool offensive = false;
    if(pw.Production(player) > pw.Production(flip(player)) && pw.NumShips(player)>pw.NumShips(flip(player)))
    {
        offensive = true;
    }
    for (int i = 0; i < not_my_planets.size(); ++i) 
    {
          Planet& p = not_my_planets[i];
          int pid = p.PlanetID();
          if(p.GrowthRate()==0) continue;
        if(offensive && p.Owner() == 0) continue;   
          int future_owner = p.FutureOwner();
          // //file<<"Future owner of this planet is "<<future_owner<<endl;
          if ( future_owner == player) 
          {
            file<<"future owner is "<<player<<endl;
              continue;
          }
          if(p.Owner() == 0  && my_half[p.PlanetID()] == false)
          {
            file<<"ditchiing "<<p.PlanetID()<<" ";
            continue;
          }

          //don't attack if enemy planet is closer to the neutral planet 
            if(turn_number == 0)
            {
                int closest_enemy = pw.ClosestPlanetByPlayer( pid, flip(player) );
                int closest_me = pw.ClosestPlanetByPlayer( pid, player );
                if(pw.Distance(closest_enemy,pid) - pw.Distance(closest_me,pid) < 0)
                {
                    continue;
                }
            }
            // //file<<"closest ally planet to the future neutral planet is "<<closest_me<<" and the closest enemy is "<<closest_enemy<<endl;
            // if(!future_owner && !FullAttack(pw, pid,player))continue;
          
         // If the planet is owned by an enemy and it is NOT a frontier planet then ignore
          // if ( p.Owner() == flip(player) &&  ( enemy_frontier.find(pid)!= enemy_frontier.end() ) ) 
          // {
          //   // //file<<"The planet is owned by enemy , but is not his frontier or future frontier, so skipping it"<<endl;
          //     continue;
          // }
          // Don't try to neutral steal planets that we would not otherwise attack
          // if ( future_owner == 2 && p.Owner() == 0 && ! enemy_frontier_future[pid] ) 
          // {
          //   // //file<<"future owner is enemy , and the planet is current neutral, plus it will be enemy future frontier, so skipping it"<<endl;
          //     continue;
          // }
          std::vector<Fleet> orders;
          // //file<<"created empty order list"<<endl;
          int score = AttackPlanet(pw,p.PlanetID(), defence_exclusions,orders, 1);
          // file<<"hello"<<endl;
          file<<"The score for attacking the planet "<<pid<<" is "<<score<<endl;
          destination_planets.push_back( std::pair<int,int>(score, pid) );
          // //file<<"pushed the target planet along with the scores into the destination_planets array"<<endl;
         // file<<"not my planets ka size is at the end of a loop "<<not_my_planets.size()<<endl;
    }

  std::sort(destination_planets.begin(), destination_planets.end());
  // //file<<"the size of destination plants for this turn is: "<<destination_planets.size()<<endl;
  vector<int> targets;
  // //file<<"the size of target planets for this turn is: "<<targets.size()<<endl;
  //file<<"the target planets for this turn are: "<<endl;
  for(int i=0;i<destination_planets.size();++i)
  {
    targets.push_back(destination_planets[i].second);
    file<<pw.GetPlanet(targets[i]).Owner()<<" ";
  }
  return targets;
}


void DoTurn(const PlanetWars& initialState, vector<Fleet>& orders) 
{
  PlanetWars pw = initialState;
	int numFleets = 1;
	bool attack =false;
   file<< "entering the turn number "<<turn_number<<endl;

  set<int> attacking_planets;
  std::vector<Planet> my_planets = pw.MyPlanets();
  std::vector<Planet> not_my_planets = pw.NotMyPlanets();
  std::vector<Planet> enemy_planets = pw.EnemyPlanets();
  std::vector<Planet> allplanets = pw.Planets();
  vector<Planet> neutral_planets = pw.NeutralPlanets();
  vector<Fleet> all_fleets = pw.Fleets();
  vector<Fleet> enemy_fleets = pw.EnemyFleets();
  map<int, int> critical_mass_map;
  int my_ships_on_planets = 0;
  int avgShips = 0;
  int player =1;
  //////////////////////////defence first///////////////////////////////////////
    Defence(pw, player);
//locked appropriate number of ships on the planets
    // now, send the ships from home planets to frontier
    // calculate potential at different frontier planets
    // send surplus ships from home to different planets, based on the potential.
    map<int, bool> frontier_map = pw.FrontierPlanets(player);
    vector<int> frontier_planets = pw.FrontierPlanetsVector(player);
    vector<int> :: iterator frontier_pointer = frontier_planets.begin();
    vector<double> potentials;
    double potential_sum =0;
    for(int i=0;i<frontier_planets.size();++i)
    {
        double value = pw.Potential(frontier_planets[i],player);
        potential_sum += value;
        potentials.push_back(value);
    }
    vector<int> not_frontier = pw.NotFrontierPlanets(player);
    for(int i=0;i<not_frontier.size();++i)
    {
        for(int j=0;j<frontier_planets.size();++j)
        {
            int size = floor( (pw.GetPlanet(not_frontier[i]).Ships(true) * potentials[j]*1.0)/(1.0*potential_sum) );
            if(size!=0 ) pw.PlaceOrder(Fleet(player, not_frontier[i] , frontier_planets[j] ,size, 0 ));
        }
    }

    DefenceExclusions defence_exclusions = AntiRage(pw, player);
  ////////////////////////////////


  // //file<<"enemy future frontier planets are:"<<endl;
  // for(map<int, bool >:: iterator i=enemy_frontier_future.begin();i!=enemy_frontier.end();++i)
  // {
  //   if(i->first == true)
  //   {
  //     //file<<i->second<<" ";
  //   }
  // }
  // //file<<endl;
    ///////////////////finding taraget planets////////////////////////////
  // file<<"not my planets ka intial size is "<<not_my_planets.size()<<endl;

  //file<<endl;


    // critical_mass_map[p.PlanetID()] = CriticalMass(p, all_fleets);
    // int p_closest = pw.ClosestPlanetByPlayer(p.PlanetID(), 1); 
    // //file<<"value of planet closest id is "<<p_closest<<std::endl;
    // for(int j=0;j<my_planets.size();++j)
    // {
    //   if(pw.Distance(p_closest ,p.PlanetID()) > pw.Distance(my_planets[j].PlanetID(), p.PlanetID()))
    //   {
    //     p_closest = my_planets[j].PlanetID();
    //   }
    // }
    // double score;
    // if(p_closest != -1){score=(1+ToSend(p, critical_mass_map))/(p.GrowthRate()*1.0 + 1)/3 + pw.Distance(p_closest, p.PlanetID())/2 ;}//*
    // else score=999999;
    // destination_planets.push_back(make_pair(p.PlanetID(), make_pair(p.Owner(),score)));
    // //file<<"done finding critical mass of not my planet "<<i<<endl;
  

  //   //file<<"done finding critical masses of not my planets"<<endl;

  //   //file<<"sorting the destinations based on the score"<<endl;

  // sort(destination_planets.begin(), destination_planets.end(), DestinationComparator());
  //   //file<<"done sorting destinations"<<endl;

  // for(int i=0;i<destination_planets.size();++i)
  // {
  //   //file<<"planet is= "<<destination_planets[i].first<<" and score is "<<destination_planets[i].second.second<<endl;
  // }

    vector<int> targets = FindTargets(pw, player, defence_exclusions);
  // file<<"the size of target planets for this turn is: "<<targets.size()<<endl;    
  Attack(pw, defence_exclusions, 1, targets);
  orders = pw.Orders();
  //file<<"the size of orders for this turn is: "<<orders.size()<<endl;

  // if(turn_number == 0)
  // {
  //   double score_next, score_curr;

  //   for(int i=0;i<destination_planets.size()-1;i++)
  //   {
  //     score_curr = destination_planets[i].second.second;
  //     score_next = destination_planets[i+1].second.second;
  //     if(score_curr == score_next)
  //     {
  //       if(pw.Distance(destination_planets[i].first , my_planets[0].PlanetID()) < pw.Distance(destination_planets[i+1].first, my_planets[0].PlanetID()))
  //       {
  //         destination_planets.erase(destination_planets.begin()+i+1);
  //       }
  //       else
  //       {
  //         destination_planets.erase(destination_planets.begin()+i);
  //       }
  //     }
  //     else
  //     {
  //       destination_planets.erase(destination_planets.begin()+i);
  //       i--;
  //     }
  //   }

  // }

  // int max_attack_size=0;
  // for(set<int> :: iterator it = attacking_planets.begin();it!= attacking_planets.end();it++)
  // {
  //   int source = *it;
  //   max_attack_size += max(pw.GetPlanet(source).NumShips() - critical_mass_map[source],0);
  // }
  // set<int>:: iterator start = attacking_planets.begin();
  // int current_buffer = pw.GetPlanet(*start).NumShips();
  // //file<<"max attack size for this turn("<<turn_number<<") is "<<max_attack_size<<endl;
  // if(myTotalShips(my_planets) > enemyTotalShips(enemy_planets, enemy_fleets) && myTotalGrowthRate(my_planets) > enemyTotalGrowthRate(enemy_planets)) 
  // {
  //   destination_planets.clear();
  //   for(int i=0;i<enemy_planets.size();++i)
  //   {
  //     destination_planets.push_back(make_pair(enemy_planets[i].PlanetID(),make_pair(0,0)));
  //   }
  // }
  // //file<<"the size of destination array is "<<destination_planets.size()<<endl;
  // for(int i=0;i<destination_planets.size() && i < 5 ;++i)
  // {
  //   const Planet& p = pw.GetPlanet(destination_planets[i].first);
  //   int One = (pw.GetPlanet(closest_three[p.PlanetID()].first())).Owner()==2?1:0;
  //   int Two = (pw.GetPlanet(closest_three[p.PlanetID()].second())).Owner()==2?1:0;
  //   int Three = (pw.GetPlanet(closest_three[p.PlanetID()].third())).Owner()==2?1:0;
  //   if(One+ Two+ Three >1 ) continue;
  //   int distant_planet = *attacking_planets.begin();
  //   double max_distance = 0;
  //   for(set<int>:: iterator it = attacking_planets.begin();it!= attacking_planets.end();++it)
  //   {
  //     if(pw.Distance(distant_planet , p.PlanetID()) < pw.Distance(*it , p.PlanetID()))
  //     {
  //       max_distance = pw.Distance(*it , p.PlanetID());
  //       distant_planet = *it;
  //     }
  //   }

  //   int remaining_ships = ToSend(p, critical_mass_map) + (p.Owner()==0?0:1)*ceil((p.GrowthRate()*max_distance));
  //   //file<<"current destination selection as "<<destination_planets[i].first<<" which has a score of "<<destination_planets[i].second.second<<" and which requires "<<remaining_ships<<" to be sent"<<endl;
  //   if(remaining_ships > max_attack_size) continue;
  //   for(set<int> :: iterator it = start;it!= attacking_planets.end();it++)
  //   {
  //     int source = *it;
  //     int max_can_send = max(0,current_buffer - critical_mass_map[source]);
  //     if(remaining_ships >= max_can_send)
  //     {
  //       remaining_ships -= max_can_send;
  //       if( max_can_send!=0){pw.IssueOrder(source, destination_planets[i].first , max_can_send);}
  //       max_attack_size -= max_can_send;
  //       //file<<"sending "<<max_can_send<<" ships from "<<source <<" to "<<destination_planets[i].first<<endl;
  //       start ++;
  //       current_buffer = pw.GetPlanet(*start).NumShips();
  //     }
  //     else
  //     {
  //       //file<<"sending "<<remaining_ships<<" ships from "<<source <<" to "<<destination_planets[i].first<<endl;
  //       max_attack_size -= remaining_ships;
  //       if(remaining_ships!=0){pw.IssueOrder(source, destination_planets[i].first , remaining_ships);}
  //       current_buffer -= remaining_ships;
  //       break;
  //     }
  //   }
  // }

  turn_number++;
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
  file.open("out.txt", ios::out);
  turn_number = 0;
  std::string current_line;
  std::string map_data;
  bool map_parsed = false;
  //file<<"starting the program"<<endl;
  while (true) {
    int c = std::cin.get();
    current_line += (char)c;
    if (c == '\n') {
      if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
        PlanetWars pw(map_data);
        if(turn_number == 0)
        {
            Planet initialPlanet = pw.MyPlanets()[0];
            Planet enemyPlanet = pw.EnemyPlanets()[0];
            vector<Planet> all_planets = pw.Planets();
            file<<"my half p "<<endl;
            for(int i=0;i< all_planets.size();++i)
            {
                if(pw.Distance(initialPlanet.PlanetID(), all_planets[i].PlanetID()) <= pw.Distance(enemyPlanet.PlanetID(), all_planets[i].PlanetID() ) )
                {
                    my_half[all_planets[i].PlanetID()] = true;
                    file<<" "<<all_planets[i].PlanetID()<<" ";
                }
                else
                {
                    my_half[all_planets[i].PlanetID()] = false;
                }

            }
        }
        file<<endl;
        // if(!map_parsed)
        // {
        //   //file<<"Calling Init"<<endl;
        //   map_parsed = true;
        //   pw.Init();
        // }
        //try  to make the above work, called only once in program lifetime
        pw.Init();
        file<<"turn_number"<<turn_number <<endl;
        map_data = "";
        std::vector<Fleet> orders;
        DoTurn(pw, orders);
        for(int i=0;i<orders.size();i++)
        {
          Fleet& f = orders[i];
            if ( f.SourcePlanet() == f.DestinationPlanet() 
                  || f.NumShips() <= 0 
                  || f.NumShips() > pw.GetPlanet(f.SourcePlanet()).Ships() 
                  || pw.GetPlanet(f.SourcePlanet()).Owner() != 1 
          ){continue;}
          pw.IssueOrder(f.SourcePlanet(), f.DestinationPlanet(), f.NumShips());
        }
        pw.FinishTurn();
      } else {
        map_data += current_line;
      }
      current_line = "";
    }
  }
  file.close();
  return 0;
}
  