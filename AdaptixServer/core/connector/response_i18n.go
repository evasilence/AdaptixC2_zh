package connector

import "github.com/gin-gonic/gin"

// payload keeps the existing wire contract ("ok" + "message") intact.
// It adds an optional "message_zh" for clients that want Chinese UI strings.
func payload(ok bool, message string) gin.H {
	h := gin.H{"ok": ok, "message": message}
	if message == "" {
		return h
	}
	if zh, ok := messageZh(message); ok {
		h["message_zh"] = zh
	}
	return h
}

func messageZh(message string) (string, bool) {
	switch message {
	case "invalid JSON data":
		return "无效的 JSON 数据", true
	case "invalid JSON structure":
		return "无效的 JSON 结构", true

	case "Server error: username not found in context":
		return "服务器错误：上下文中未找到用户名", true
	case "Server error: invalid username type in context":
		return "服务器错误：上下文中的用户名类型无效", true

	case "Invalid listener name":
		return "监听器名称无效", true
	case "Listener started":
		return "监听器已启动", true
	case "Listener stopped":
		return "监听器已停止", true
	case "Listener Edited":
		return "监听器已更新", true

	case "sync started":
		return "同步已开始", true

	case "agent_id is required":
		return "agent_id 为必填项", true
	case "limit must be an integer":
		return "limit 必须是整数", true
	case "limit must be >= 0":
		return "limit 必须 >= 0", true
	case "offset must be an integer":
		return "offset 必须是整数", true
	case "offset must be >= 0":
		return "offset 必须 >= 0", true

	case "l_host is required":
		return "l_host 为必填项", true
	case "t_host is required":
		return "t_host 为必填项", true
	case "username is required":
		return "username 为必填项", true
	case "password is required":
		return "password 为必填项", true
	case "l_port must be from 1 to 65535":
		return "l_port 必须在 1 到 65535 之间", true
	case "t_port must be from 1 to 65535":
		return "t_port 必须在 1 到 65535 之间", true
	case "port must be from 1 to 65535":
		return "port 必须在 1 到 65535 之间", true

	case "Tunnel stopped":
		return "隧道已停止", true
	case "Tunnel info updated":
		return "隧道信息已更新", true
	}

	return "", false
}
