#include "q3psvr2_sense_bridge.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    q3psvr2_sense_sample s={0},out;
    q3psvr2_sense_reset();
    assert(q3psvr2_sense_get_pose(0, 200, 50, &out)==0);
    s.timestamp_us=1000;s.pose_valid=1;s.position_m[0]=1.25f;
    q3psvr2_sense_publish(1,&s);
    assert(q3psvr2_sense_get_pose(1,1030,50,&out)==1 && out.position_m[0]==1.25f);
    assert(q3psvr2_sense_get_pose(1,1100,50,&out)==0); /* must not use stale hand */
    assert(q3psvr2_sense_get_pose(0,1030,50,&out)==0);
    puts("sense bridge fallback/timeout test passed");
    return 0;
}
