/**
 * OneNet 物模型 API 封装
 * - 设备属性最新数据查询（GET  query-device-property,        doc/1421）
 * - 获取设备属性详情（POST     query-device-property-detail,  doc/1419，需设备在线）
 * - 设置设备属性（POST         set-device-property,           doc/1418，需设备在线）
 */
import { ONENET_CONFIG } from './config.js'
import { generateProductAuthorization } from './crypto.js'

function authHeader() {
	return {
		'content-type': 'application/json',
		authorization: generateProductAuthorization(
			ONENET_CONFIG.PRODUCT_ID,
			ONENET_CONFIG.ACCESS_KEY,
			ONENET_CONFIG.TOKEN_TTL
		)
	}
}

/**
 * 统一请求：HTTP 2xx 且业务码 code === 0 才算成功
 */
function request(method, url, data) {
	return new Promise((resolve, reject) => {
		uni.request({
			url,
			method,
			data,
			header: authHeader(),
			timeout: 10000,
			success: (res) => {
				const body = res.data || {}
				if (res.statusCode === 200 && body.code === 0) {
					resolve(body)
					return
				}
				let msg = body.msg || ('HTTP ' + res.statusCode)
				if (res.statusCode === 401 || res.statusCode === 403) {
					msg = '鉴权失败，请检查产品密钥'
				}
				reject({
					statusCode: res.statusCode,
					code: body.code,
					msg,
					requestId: body.request_id,
					raw: body
				})
			},
			fail: (err) => {
				reject({
					statusCode: -1,
					msg: err.errMsg || '网络请求失败',
					raw: err
				})
			}
		})
	})
}

/**
 * 设备属性最新数据查询（平台缓存的最新上报值，不要求设备在线应答）
 * 返回归一化后的 { identifier: { value, time, dataType, accessMode, name } }
 */
export function queryLatestProperties() {
	const url = ONENET_CONFIG.BASE_URL +
		'/thingmodel/query-device-property?product_id=' + encodeURIComponent(ONENET_CONFIG.PRODUCT_ID) +
		'&device_name=' + encodeURIComponent(ONENET_CONFIG.DEVICE_NAME)
	return request('GET', url).then(normalizeLatest)
}

/**
 * 获取设备属性详情（向在线设备实时下发查询）
 * @param {String[]} identifiers 功能点标识数组
 * 返回 { identifier: value }
 */
export function queryPropertyDetail(identifiers) {
	const url = ONENET_CONFIG.BASE_URL + '/thingmodel/query-device-property-detail'
	return request('POST', url, {
		product_id: ONENET_CONFIG.PRODUCT_ID,
		device_name: ONENET_CONFIG.DEVICE_NAME,
		params: identifiers
	}).then(body => parseValueMap(body.data || {}))
}

/**
 * 设备属性记录查询（历史上报数据，doc/1422）
 * @param {String} identifier 属性功能点标识（单次查一个）
 * @param {Number} startTime 起始时间（毫秒时间戳）
 * @param {Number} endTime   结束时间（毫秒时间戳）
 * @param {Number} [limit=100] 记录数，范围 1-100
 * @returns Promise<Array<{value:*, time:Number}>> 已按时间正序排列
 */
export function queryPropertyHistory(identifier, startTime, endTime, limit) {
	const url = ONENET_CONFIG.BASE_URL +
		'/thingmodel/query-device-property-history?product_id=' + encodeURIComponent(ONENET_CONFIG.PRODUCT_ID) +
		'&device_name=' + encodeURIComponent(ONENET_CONFIG.DEVICE_NAME) +
		'&identifier=' + encodeURIComponent(identifier) +
		'&start_time=' + startTime +
		'&end_time=' + endTime +
		'&sort=1&limit=' + (limit || 100)
	return request('GET', url).then(body => {
		const list = Array.isArray(body.data)
			? body.data
			: (body.data && Array.isArray(body.data.list) ? body.data.list : [])
		// sort=1 服务端正序；兜底再排一次
		return list.map(it => ({
			value: parseJsonValue(it.value),
			time: Number(it.time) || 0
		})).filter(it => it.time > 0).sort((a, b) => a.time - b.time)
	})
}

/**
 * 设置设备属性（向在线设备下发设置命令）
 * @param {Object} params 形如 { fanSpeed: 60 }
 */
export function setDeviceProperty(params) {
	const url = ONENET_CONFIG.BASE_URL + '/thingmodel/set-device-property'
	return request('POST', url, {
		product_id: ONENET_CONFIG.PRODUCT_ID,
		device_name: ONENET_CONFIG.DEVICE_NAME,
		params
	}).then(body => {
		// 设备端回复 code 200 才表示设备真正执行成功
		if (body.data && body.data.code !== undefined && body.data.code !== 200) {
			return Promise.reject({
				code: body.data.code,
				msg: body.data.msg || '设备执行设置失败'
			})
		}
		return body
	})
}

/**
 * 把 1421 接口返回的 list 归一化（文档写 data.list，示例为 data 数组，两种都兼容）
 */
function normalizeLatest(body) {
	const list = Array.isArray(body.data)
		? body.data
		: (body.data && Array.isArray(body.data.list) ? body.data.list : [])
	const map = {}
	list.forEach(item => {
		map[item.identifier] = {
			value: parseJsonValue(item.value),
			time: Number(item.time) || 0,
			dataType: item.data_type,
			accessMode: item.access_mode,
			name: item.name
		}
	})
	return map
}

/**
 * 1419 接口返回的 data 值可能是裸值，这里同样做一次 JSON 字符串兜底解析
 */
function parseValueMap(data) {
	const map = {}
	Object.keys(data).forEach(k => {
		map[k] = parseJsonValue(data[k])
	})
	return map
}

/**
 * OneNet 的 value 通常是 JSON 字符串（如 "42.5"、"\"abc\""），按 JSON 解析；
 * 若本身已经是 number/boolean/object 则直接返回，解析失败按原始字符串返回
 */
function parseJsonValue(raw) {
	if (raw === undefined || raw === null) return ''
	if (typeof raw !== 'string') return raw
	try {
		return JSON.parse(raw)
	} catch (e) {
		return raw
	}
}
