

using System.Collections.Generic;
using UnityEngine;

namespace c2s
{
    public class Vec3
    {
        public float x = 0;
        public float y = 0;
        public float z = 0;
        public Vec3() { }

        public Vec3(Vector3 vector3)
        {
            this.x = vector3.x;
            this.y = vector3.y;
            this.z = vector3.z;
        }
    }
    
    public class Vec2
    {
        public float x = 0;
        public float y = 0;
    }
    
    public class Sync
    {
        public long player_id = 0;
        public List<Vec2> inputs = new List<Vec2>();
    }

    public class CreateMessage
    {
        public long player_id = 0;
        public Vec3 pos = new Vec3();
        public Vec3 rot = new Vec3();
    }
}

namespace s2c
{
    public class LoginRequest
    {
        
    }
    public class LoginResponse
    {
        public long player_id;
    }
}