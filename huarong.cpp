#include <iostream>
#include <queue>
#include <vector>
#include <unordered_set>

using namespace std;

#define WIDTH 4
#define HEIGHT 5
#define SIZE ((WIDTH) * (HEIGHT))


#define DIRECT_UP 0
#define DIRECT_DOWN 1
#define DIRECT_LEFT 2
#define DIRECT_RIGHT 3


#define INDEX_BBOX 0
#define INDEX_VBOX 1
#define INDEX_HBOX 2
#define INDEX_SBOX 3

#define BBOX_W 2
#define BBOX_H 2
#define VBOX_W 1
#define VBOX_H 2
#define HBOX_W 2
#define HBOX_H 1
#define SBOX_W 1
#define SBOX_H 1


#define POINT_MOVE(p,i,j) ((p) << (WIDTH * (i) + j)) 
#define POINT(index) (1 << (index))
#define POINT_REMOVE(chart, shape) ((chart) & ~(shape))
#define POINT_REPLACE(chart, osp, nsp) ((chart) & ~(osp) | (nsp))

#define DEBUE false

class Border
{
private:
    int positions[4];
public:
    friend class PointUtil;
    Border(int width, int height)
    {
        int left = 0;
        int right =0;
        int up = 0;
        int down = 0;
        for (int i = 0; i <= HEIGHT - height; i++)
        {
            left = left | POINT_MOVE(1, i, 0);
            right = right | POINT_MOVE(1, i, WIDTH - width);
        }
        for (int j = 0; j <= WIDTH - width; j++)
        {
            up = up | POINT_MOVE(1, 0, j);
            down = down | POINT_MOVE(1, HEIGHT-height, j);
        }
        positions[DIRECT_UP] = up;
        positions[DIRECT_DOWN] = down;
        positions[DIRECT_LEFT] = left;
        positions[DIRECT_RIGHT] = right;
    }
    bool at(int direct, int point)
    {
        return ((positions[direct]) & point) > 0;
    }
    int operator[](int direct) const
    {
        return positions[direct];
    }
};

struct State
{
public:
    vector<int> values;
public:
    int step;
    int index;
    int direct;
    int id;
    int parentId;
    const State* parent;
    static int  ID;
public:
    State():values(4),parent(NULL),step(0),id(0),parentId(-1)
    {
        for (int i = 0; i < 4; i++)
            values[i] = 0;
        //this->id = (ID++);
    }
    State(const State &state)
    {
         if(this == &state)
            return;
         values=state.values;
         step = 0;
         id = 0;
         parent = NULL;
         this->id = (++ID);
         
    }
    int operator[](int index) const
    {
        return values[index];
    }
    int& operator[](int index)
    {
        return values[index];
    }

    bool arrive(int index, int i, int j) const
    {
        return ((*this)[index] & POINT_MOVE(1, i, j)) > 0;
    }
    void fill(int index, int i, int j)
    {
        (*this)[index] = (*this)[index] | POINT_MOVE(1, i, j);
    }
};
int State::ID = 0;


class Pannel
{
private:
    vector<vector<char> > chart;

public:
    friend class PointUtil;
    Pannel()
    {
        chart = vector<vector<char> >();
        chart.reserve(HEIGHT);
        for (int i = 0; i < HEIGHT; i++)
        {
            chart.push_back(vector<char>(WIDTH,'-'));
        } 
    }
    void show()
    {
        for(int i = 0; i < chart.size(); i++)
        {
            for (int j = 0; j < chart[i].size(); j++)
            {
                cout<<chart[i][j] <<" ";
            }
            cout<<endl;
        }
    }
};

class PointUtil
{

public:
    static void draw(int positions, char ch, Pannel &pannel)
    {
        for (int n = 0; n < SIZE; n++)
        {
            if ((positions & (1 << n)) > 0)
            {
                int x = n / WIDTH;
                int y = n % WIDTH;
                pannel.chart[x][y] = ch;
            }
        }   
    };
    
    static void draw(const Border &border, char ch, Pannel &pannel)
    {
        draw(border[DIRECT_UP], ch, pannel);
        draw(border[DIRECT_DOWN], ch, pannel);
        draw(border[DIRECT_LEFT], ch, pannel);
        draw(border[DIRECT_RIGHT], ch, pannel);
    };


    static void draw(int positions, int width, int height, char ch, Pannel &pannel)
    {
        for (int n = 0; n < SIZE; n++)
        {
            if ((positions & (1 << n)) > 0)
            {
                int x = n / WIDTH;
                int y = n % WIDTH;
                for (int i = 0; i < height; i++)
                {
                    for (int j = 0; j < width; j++)
                    {
                        pannel.chart[x+i][y+j] = ch;
                    }
                } 
            }
        } 
    }; 

    static void draw(const State &state, Pannel &pannel)
    {
        draw(state[INDEX_BBOX], BBOX_W, BBOX_H, '1' - 1 + INDEX_BBOX, pannel);
        draw(state[INDEX_VBOX], VBOX_W, VBOX_H, '1' - 1 + INDEX_VBOX, pannel);
        draw(state[INDEX_HBOX], HBOX_W, HBOX_H, '1' - 1 + INDEX_HBOX, pannel);
        draw(state[INDEX_SBOX], SBOX_W, SBOX_H, '1' - 1 + INDEX_SBOX, pannel);
    };

    static void draw_point(int positions, Pannel &pannel)
    {
        draw(positions, 0, 0,'X', pannel);
    }


    static void fill(int positions, int width, int height, int &chart)
    {
        for (int n = 0; n < SIZE; n++)
        {
            //if ((positions & (1 << n)) > 0)
            //{
             //   int x = n / WIDTH;
             //   int y = n % WIDTH;
                for (int i = 0; i < height; i++)
                {
                    for (int j = 0; j < width; j++)
                    {
                        chart = chart | POINT_MOVE(positions, i, j) ;
                    }
                } 
            //}
        } 
    }; 

    static void fill(const State &state, int &chart)
    {
        fill((state)[INDEX_BBOX], BBOX_W, BBOX_H, chart);
        fill((state)[INDEX_VBOX], VBOX_W, VBOX_H, chart);
        fill((state)[INDEX_HBOX], HBOX_W, HBOX_H, chart);
        fill((state)[INDEX_SBOX], SBOX_W, SBOX_H, chart); 
    };


    static int move(int direct, int point)
    {
        if (direct == DIRECT_UP)
        {
            return ((point)>>(WIDTH));
        }
        else if (direct == DIRECT_DOWN)
        {
            return ((point)<<(WIDTH));
        }
        else if (direct == DIRECT_LEFT)
        {
            return ((point)>>(1));
        }
        else if (direct == DIRECT_RIGHT)
        {
            return ((point)<<(1));
        }
        exit(1);
    };
};

struct LOG
{
    static void debug(const State &state)
    {
        if (false)
        {
            Pannel pannel;
            cout<<"step-"<<state.step<<",id-"<<(state.id)<<",parent-"<<state.parentId<<endl;
            PointUtil::draw(state, pannel);
            pannel.show();
        }
    }
    static void debug(const Border &border)
    {
        Pannel pannel;
        //cout<<"id-"<<(state.id)<<",parent-"<<state.parentId<<endl;
        PointUtil::draw(border, 'X', pannel);
        pannel.show();
    }
    static void debug(const int &points)
    {
        Pannel pannel;
        //cout<<"id-"<<(state.id)<<",parent-"<<state.parentId<<endl;
        PointUtil::draw(points, 'X', pannel);
        pannel.show();
    }
    static void info(const State &state)
    {
        //cout<<"id-"<<(state.id)<<",parent-"<<state.parentId<<endl;
            Pannel pannel;
            cout<<"step-"<<state.step<<",id-"<<(state.id)<<",parent-"<<state.parentId<<endl;
            PointUtil::draw(state, pannel);
            pannel.show();
    }
    //static void debug()
};

struct Shape
{
    int width;
    int height;

    int index;
    Border border;

    Shape(int w, int h):width(w),height(h),border(w, h)
    {
        //LOG::debug(border);
    }

    

    void move(const State *state, int &chart, vector<State*> &results)
    {
        int positions = (*state)[this->index];
        int i = 0;
        for (; i < SIZE; i++)
        {
            int position = POINT(i);
            if ((positions & position) <= 0)
            {
                continue;
            }
            for (int direct = 0; direct < 4; direct++)
            {

                if (border.at(direct, position)){
                    continue;
                }
                int new_position = PointUtil::move(direct, position); //= move(direct, position);

               // LOG::info(position);
                //LOG::info(new_position);
                //allow_move
                if (!can_move(position, new_position, chart)){
                    continue;
                }
                
                State* new_state = new State((*state));

                //cout<<"state->id "<<(state->id)<<endl;

                new_state->step = (state->step + 1);
                new_state->direct = direct;
                new_state->index = index;
                new_state->parentId = state->id;
                new_state->parent = state;
                //new_state->id = State::ID++;
                //new_state->parent = state;
                //new_state->parentId = state->id;
                (*new_state)[index] = POINT_REPLACE((*new_state)[index], position, new_position);
                //cout<<" new state"<<endl;
                //LOG::debug(*new_state);
                //log_postions(chart);
                //log_postions((*new_state)[index]);
                results.push_back(new_state); 
            }
        }
                 
    }

    bool can_move(int source_position, int target_position, int chart)
    {
        int source_shape = 0;
        PointUtil::fill(source_position, width, height, source_shape);

        int target_shape = 0;
        PointUtil::fill(target_position, width, height, target_shape);
        //cout<<"w "<<width <<" h "<<height<<" "<<index<<endl;
        // LOG::info(source_shape);
        // LOG::info(target_shape);
        // LOG::info(chart);
        // LOG::info(chart & ~source_shape & target_shape);
        return (chart & ~source_shape & target_shape) == 0;

    }
};

struct  BBox:Shape
{
    BBox():Shape(BBOX_W, BBOX_H)
    {
        this->index = INDEX_BBOX;
    }
    
};

struct VBox:Shape
{
    VBox():Shape(VBOX_W, VBOX_H)
    {
        this->index = INDEX_VBOX;
    }
};

struct HBox:Shape
{
    HBox():Shape(HBOX_W, HBOX_H)
    {
        this->index = INDEX_HBOX;
    }
};

struct  SBox:Shape
{
    SBox():Shape(SBOX_W, SBOX_H)
    {
        this->index = INDEX_SBOX;
    }
    
};


class StateHandler
{

private:
    BBox bbox;
    VBox vbox;
    HBox hbox;
    SBox sbox;

public:
     

    vector<State*> next(State *state)
    {
        vector<State*> results;
        int chart = 0;
        PointUtil::fill(*state, chart);
        bbox.move(state, chart, results);
        vbox.move(state, chart, results);
        hbox.move(state, chart, results);
        sbox.move(state, chart, results);
        return results;
    }

     


};

 struct Equal
{

    bool operator()(const State *s1, const State *s2) const
    {
        for(int i = 0; i < 4; i++)
        {
            if ((*s1)[i] != (*s2)[i]) return false;
        }
        return true;
    }
};

struct Hash
{
public:
    int operator()(const State *state) const
    {
         int h = (*state)[0] ^ ((*state)[1] << 1) ^ ((*state)[2] << 2) ^ ((*state)[3] << 3) ; 
         return h;
    }
};

class Soluation
{
private:
    queue<State*> queue;
    unordered_set<const State*,Hash,Equal> set;
    StateHandler state_handler;


public:
    template <class Condition>
    const State* search(const State &start_state, Condition cond)
    {
        State* init_state = new State(start_state);
        queue.push(init_state);
        set.insert(init_state);

        while (!queue.empty())
        {
            State* cur_state = queue.front();
            queue.pop();
            vector<State*> states = state_handler.next(cur_state);
            if (states.size() > 0)
            {
                for (int i = 0; i < states.size(); i++)
                {
                    State* direct_state = states[i];
                    if (0 < set.count(direct_state)){
                        delete direct_state;
                        continue;
                    } 
                    else
                    {
                        //LOG::info((*direct_state));
                    }
                    LOG::debug(*direct_state);
                    if (cond.call(*direct_state))//->arrive(0, 3, 1))
                    {
                        record_path(direct_state, set);
                        //LOG::debug(*new_state);
                        return direct_state;
                    }
                    set.insert(direct_state);
                    queue.push(direct_state);
                }
            }
        }
        return NULL;
    }

    void record_path(const State *state, unordered_set<const State*,Hash,Equal> &set )
    {
        //vector<const State*> results;
        while(state != NULL)
        {
            //results.push_back(state);
            set.erase(state);
            state = state->parent;
        }

        //cout<<"size "<<results.size()<<endl;

        // for (int i = results.size() - 1; i > 0; i--)
        // {
        //     LOG::debug((*results[i]));
        // }

        for (unordered_set<const State*,Hash,Equal>::iterator it = set.begin(); it != set.end(); it++)
        {
            delete (*it);
        }

        
    }


};

struct Condition
{
    bool call(const State &state)
    {
        return state.arrive(INDEX_BBOX, 3, 1);
    }
};
int main()
{
    State state;
    state.fill(0, 0, 1);

    state.fill(1, 0, 0);
    state.fill(1, 0, 3);
    state.fill(1, 2, 0);
    state.fill(1, 2, 3);

    state.fill(2, 2, 1);

    state.fill(3, 4, 0);
    state.fill(3, 3, 1);
    state.fill(3, 3, 2);
    state.fill(3, 4, 3);

    Soluation sol;
    Condition cond;
    //LOG::debug(state);

    const State *target_state = sol.search(state,cond);
    while(target_state != NULL)
    {
        while(target_state != NULL)
        {
            //results.push_back(state);
            //set.erase(state);
            LOG::info((*target_state));
            target_state = target_state->parent;
        }
    }
    //cout<<sol.search(state,cond)<<endl;


     return 0;
}