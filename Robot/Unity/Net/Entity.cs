using System;
using System.Collections.Generic;
using c2s;
using rpc;
using UnityEngine;

public class Entity : MonoBehaviour
{
    public const int STATUS_NONE = 0; //静止状态
    public const int STATUS_MOVE = 1; //静止状态
    public const int STATUS_SKILL = 2; //释放技能状态
    public const int STATUS_JUMP = 3; //跳跃技能状态
    
    private int frame = 0;
    private int maxFrame = 0;
    private Rigidbody _rigidbody;
    private List<Vector3> frameBuffer = new List<Vector3>();
    //private Dictionary<int, Vector3> frameBuffer = new Dictionary<int, Vector3>();
    private void Start()
    {
        this._rigidbody = this.GetComponent<Rigidbody>();
    }
    private long lastFrameUpdateTime = 0;

    private void Update()
    {
        if (this.frameBuffer.Count == 0)
        {
            return;
        }
        
        if (this.frameBuffer.Count > this.frame)
        {
            Vector3 pos = this.frameBuffer[this.frame];
            if (!pos.Equals(Vector3.zero))
            {
                this._rigidbody.velocity = pos * 2;
                Quaternion targetRotation = Quaternion.LookRotation(-pos.normalized);
                this.transform.rotation = Quaternion.Lerp(this.transform.rotation, targetRotation, 0.08f);
            }

            //Debug.Log($"frame:{this.frame} => {pos.x}:{pos.y}");
            this.frame++;
        }
    }

    public void OnChange(c2s.Sync syncInfo)
    {
        foreach (Vec2 vec2 in syncInfo.inputs)
        {
            this.frameBuffer.Add(new Vector3()
            {
                x = vec2.x,
                z = vec2.y
            });
        }
    }
}
