package connector

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

func (tc *TsConnector) TcCredentialsList(ctx *gin.Context) {
	jsonCreds, err := tc.teamserver.TsCredentilsList()
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, err.Error()))
		return
	}

	ctx.Data(http.StatusOK, "application/json; charset=utf-8", []byte(jsonCreds))
}

type CredsAdd struct {
	Username string `json:"username"`
	Password string `json:"password"`
	Realm    string `json:"realm"`
	Type     string `json:"type"`
	Tag      string `json:"tag"`
	Storage  string `json:"storage"`
	Host     string `json:"host"`
}

func (tc *TsConnector) TcCredentialsAdd(ctx *gin.Context) {
	var m map[string]interface{}
	var creds []map[string]interface{}

	if err := ctx.ShouldBindJSON(&m); err != nil {
		ctx.JSON(http.StatusOK, payload(false, "invalid JSON data"))
		return
	}
	arr, ok := m["creds"].([]interface{})
	if !ok {
		ctx.JSON(http.StatusOK, payload(false, "invalid JSON structure"))
		return
	}
	for _, v := range arr {
		if obj, ok := v.(map[string]interface{}); ok {
			creds = append(creds, obj)
		}
	}

	err := tc.teamserver.TsCredentilsAdd(creds)
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, err.Error()))
		return
	}

	ctx.JSON(http.StatusOK, payload(true, ""))
}

type CredsEdit struct {
	CredId   string `json:"cred_id"`
	Username string `json:"username"`
	Password string `json:"password"`
	Realm    string `json:"realm"`
	Type     string `json:"type"`
	Tag      string `json:"tag"`
	Storage  string `json:"storage"`
	Host     string `json:"host"`
}

func (tc *TsConnector) TcCredentialsEdit(ctx *gin.Context) {
	var credsEdit CredsEdit
	err := ctx.ShouldBindJSON(&credsEdit)
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, "invalid JSON data"))
		return
	}

	err = tc.teamserver.TsCredentilsEdit(credsEdit.CredId, credsEdit.Username, credsEdit.Password, credsEdit.Realm, credsEdit.Type, credsEdit.Tag, credsEdit.Storage, credsEdit.Host)
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, err.Error()))
		return
	}

	ctx.JSON(http.StatusOK, payload(true, ""))
}

type CredsTag struct {
	CredIdArray []string `json:"id_array"`
	Tag         string   `json:"tag"`
}

func (tc *TsConnector) TcCredentialsSetTag(ctx *gin.Context) {
	var (
		credsTag CredsTag
		err      error
	)

	err = ctx.ShouldBindJSON(&credsTag)
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, "invalid JSON data"))
		return
	}

	err = tc.teamserver.TsCredentialsSetTag(credsTag.CredIdArray, credsTag.Tag)

	ctx.JSON(http.StatusOK, payload(true, ""))
}

type CredsRemove struct {
	CredsId []string `json:"cred_id_array"`
}

func (tc *TsConnector) TcCredentialsRemove(ctx *gin.Context) {
	var credsRemove CredsRemove
	err := ctx.ShouldBindJSON(&credsRemove)
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, "invalid JSON data"))
		return
	}

	err = tc.teamserver.TsCredentilsDelete(credsRemove.CredsId)
	if err != nil {
		ctx.JSON(http.StatusOK, payload(false, err.Error()))
		return
	}

	ctx.JSON(http.StatusOK, payload(true, ""))
}
