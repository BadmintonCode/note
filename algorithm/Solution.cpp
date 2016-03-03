/*

棋盘一共有20个空格。

坐标或者点：
    a)整数对<x,y>,（x=第几行，y=第几列）表示
    b)整数x，其中x=1<<n,n表示棋盘上第几个点（n>=1 && n<=20）表示

棋子坐标：
    每个棋子左上角占的点表示该棋子的坐标。

点或者坐标集合：
    将每个点或者坐标代表的整数position进行与运算（&）后表示

棋子空间的点集：
    棋子自身空间构成的点（一个棋子对应多个点），组成的集合，也用整数表示。


棋盘布局：
    见State，4个整数，每个整数表示一种棋子坐标的集合，用&运算判断棋子是否到达某个点。

棋子可移动边界：
    见Border，4个整数，表示棋子移动时，4个方向上能到达的坐标边界（坐标边界也是点的集合）

棋子移动：
    棋子移动时候，需要判断4个方向是否可以移动，这里将所有棋子构成的点的集合（即整数）
    进行与非等运算来判断。
    见PointUtil::fill/Shape::can_move等


算法：
  使用BFS，用State表示状态，使用map记录遍历过的状态去重。


*/

#include <iostream>
#include <queue>
#include <vector>
#include <unordered_set>

using namespace std;

#define WIDTH 4             /*棋盘宽度*/
#define HEIGHT 5            /*棋盘高度*/
#define SIZE ((WIDTH) * (HEIGHT))


#define DIRECT_UP 0       /*方向向上，下类似*/
#define DIRECT_DOWN 1
#define DIRECT_LEFT 2
#define DIRECT_RIGHT 3


#define INDEX_BBOX 0    /*棋子的index*/
#define INDEX_VBOX 1
#define INDEX_HBOX 2
#define INDEX_SBOX 3

#define BBOX_W 2        /*棋子（曹操）的高度，下类似*/
#define BBOX_H 2
#define VBOX_W 1
#define VBOX_H 2
#define HBOX_W 2
#define HBOX_H 1
#define SBOX_W 1
#define SBOX_H 1


#define POINT_MOVE(p,i,j) ((p) << (WIDTH * (i) + j)) /*坐标转换：整数对 转换成 整数*/
#define POINT(index) (1 << (index))                  //
#define POINT_REMOVE(chart, shape) ((chart) & ~(shape))  /*棋盘 移除某个 棋子*/
#define POINT_REPLACE(chart, osp, nsp) ((chart) & ~(osp) | (nsp)) /*棋盘 移动某个 棋子*/

#define DEBUE false

class Border
{
private:
    int positions[4];
public:
    friend class PointUtil;
    Border(int width, int height)//根据棋子的宽高，计算棋子可以移动的边界
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
/*
当前棋面的状态
有个数组成员，数组长度为4，分别表示4类棋子在棋盘占据的坐标

*/
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
    /*
    * index 对应的棋子是否到达<i,j>
    */
    bool arrive(int index, int i, int j) const
    {
        return ((*this)[index] & POINT_MOVE(1, i, j)) > 0;
    }
    /*
    * index 对应的棋子放在<x,y>
    * 
    */
    void fill(int index, int i, int j)
    {
        (*this)[index] = (*this)[index] | POINT_MOVE(1, i, j);
    }
};
int State::ID = 0;

/*
辅助类，用于画出当前棋盘的布局
*/
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
/*
坐标的工具类
*/
class PointUtil
{

public:
    /*
    * 把点集 positions 在pannel上用字符 ch  画出来
    */
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
    
    /*
    * 把边界 border 在pannel上用字符 ch  画出来
    */
    static void draw(const Border &border, char ch, Pannel &pannel)
    {
        draw(border[DIRECT_UP], ch, pannel);
        draw(border[DIRECT_DOWN], ch, pannel);
        draw(border[DIRECT_LEFT], ch, pannel);
        draw(border[DIRECT_RIGHT], ch, pannel);
    };

    /*
    * 把多个棋子（宽度width，高度height）构成的点（左上角）集，在pannel上用字符 ch  画出来
    */
    static void draw(int positions, int width, int height, char ch, Pannel &pannel)
    {
        for (int n = 0; n < SIZE; n++)
        {
            if ((positions & (1 << n)) > 0)//第 n 个位置放置了棋子
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

    /*
    * 把状态 state 在pannel 画出，每个棋子用index 对应的字符画出
    */
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


    /*
    * 把某个类型，数量多个的棋子（宽度width，高度height）坐标构成的点集，用整数表示
    * positions ： 多个棋子坐标的点集
    * width ： 棋子宽度
    * height ： 棋子高度
    * chart ： 多个棋子空间点集
    */
    static void fill(int positions, int width, int height, int &chart)
    {
        //for (int n = 0; n < SIZE; n++)
        //{
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
        //} 
    }; 

    /**
    * 计算某个状态state下，所有棋子空间的点集
    */
    static void fill(const State &state, int &chart)
    {
        fill((state)[INDEX_BBOX], BBOX_W, BBOX_H, chart);
        fill((state)[INDEX_VBOX], VBOX_W, VBOX_H, chart);
        fill((state)[INDEX_HBOX], HBOX_W, HBOX_H, chart);
        fill((state)[INDEX_SBOX], SBOX_W, SBOX_H, chart); 
    };

    /**
    * 某个点 或者 点集 移动
    */
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
/*
棋子基类
*/
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

    
    /**
    * 某个状态下，当前所有棋子空间点集为chart， 可以移动index 对应的棋子 ，达到的状态放在 results 中
    */
    void move(const State *state, int &chart, vector<State*> &results)
    {
        int positions = (*state)[this->index]; //获取index 所有棋子的点集
        int i = 0;
        for (; i < SIZE; i++)
        {
            int position = POINT(i);
            if ((positions & position) <= 0)
            {
                continue;
            }
            for (int direct = 0; direct < 4; direct++)//在4个方向移动其中的一个棋子
            {

                if (border.at(direct, position)){ //当前已经在边界
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
    /**
    * 在某个状态下，所有棋子空间的点集为chart，判断棋子index 是否可以从 source_position 移动到 target_position
    */
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
        return (chart & ~source_shape & target_shape) == 0; // 判断移动后是否与其他棋子不重合

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
     
    /*
    * 根据当前状态 state ，选择棋盘上所有的棋子进行移动。
    */
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
    unordered_set<const State*,Hash,Equal> set;   /*用来记录已经遍历过的状态*/  
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