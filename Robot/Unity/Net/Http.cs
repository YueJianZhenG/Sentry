using System.Net.Http;
using System.Text;
using ETModel;
using UnityEngine;
using UnityEngine.Networking;

namespace http
{
    public static class HttpHelper
    {
        public static async ETTask<T> Get<T>(string url) where T : class
        {
            ETTaskCompletionSource taskCompletionSource = new ETTaskCompletionSource();
            using (UnityWebRequest www = UnityWebRequest.Get(url))
            {
                // www.SetRequestHeader("Authorization", LoginHelper.WebToken);
                www.SendWebRequest().completed += (AsyncOperation e) => { taskCompletionSource.SetResult(); };
                await taskCompletionSource.Task;
                if (www.result == UnityWebRequest.Result.ConnectionError ||
                    www.result == UnityWebRequest.Result.ProtocolError)
                {
                    Debug.LogError($"Error: {www.error}");
                    return null;
                }
                string data = www.downloadHandler.text;
                return JsonUtility.FromJson<T>(data);
            }
        }

        public static async ETTask<T> Post<T>(string url, object body) where T : class
        {
            string request = JsonUtility.ToJson(body);
            ETTaskCompletionSource taskCompletionSource = new ETTaskCompletionSource();
            using (UnityWebRequest www = new UnityWebRequest(url, UnityWebRequest.kHttpVerbPOST))
            {
                byte[] bodyRaw = Encoding.UTF8.GetBytes(request);
                www.uploadHandler = new UploadHandlerRaw(bodyRaw);
                www.downloadHandler = new DownloadHandlerBuffer();
                www.SetRequestHeader("Content-Type", "application/json");
                //www.SetRequestHeader("Authorization", LoginHelper.WebToken);
                www.SendWebRequest().completed += e => { taskCompletionSource.SetResult(); };
                await taskCompletionSource.Task;
                if (www.result == UnityWebRequest.Result.ConnectionError ||
                    www.result == UnityWebRequest.Result.ProtocolError)
                {
                    Debug.LogError($"Error: {www.error}");
                    return null;
                }
                string data = www.downloadHandler.text;
                return JsonUtility.FromJson<T>(data);
            }
        }
    }
}