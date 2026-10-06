/**
 * OneNet 设备接入配置
 * 正式上线时，产品 AccessKey 属于敏感信息，建议放到业务服务器，由服务器签发 token，
 * App 端不要直接内置产品密钥。
 */
export const ONENET_CONFIG = {
	// OneNet 新版 API 域名
	BASE_URL: 'https://iot-api.heclouds.com',
	// 产品ID
	PRODUCT_ID: '2Its5wq8a3',
	// 设备名称
	DEVICE_NAME: 'lot_device',
	// 产品级 AccessKey
	ACCESS_KEY: 'czkTF5mpf5FrcQ+7hT99aeeJG5V7LQdF113JPQvpcQE=',
	// token 有效期（秒）
	TOKEN_TTL: 3600,
	// 最新数据轮询间隔（毫秒）
	POLL_INTERVAL: 5000
}
