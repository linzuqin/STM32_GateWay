/**
 * OneNet 设备数据共享 store（三页面共用）
 *
 * 两个频率概念分开：
 *  - 拉取频率：POLL_INTERVAL（5s），保证实时卡片新鲜
 *  - 趋势采样：启动时用 1422 历史接口回填一次（30 分钟内最多 100 点），
 *    之后每次轮询落一个点（按设备上报时间戳去重），保留最近 100 个点
 */
import { reactive } from 'vue'
import { ONENET_CONFIG } from './config.js'
import { queryLatestProperties, queryPropertyDetail, queryPropertyHistory, setDeviceProperty } from './api.js'

// 属性元数据，对应 OneNet 物模型 properties
export const PROP_META = [
	{ identifier: 'BoardBOOT', name: '启动次数', dataType: 'int32', icon: '启', unit: '次', decimals: 0, color: '#ffa94d', metric: true },
	{ identifier: 'BoardTemp', name: '板载温度', dataType: 'float', icon: '板', unit: '°C', decimals: 1, color: '#ff6b6b', metric: true, chart: true },
	{ identifier: 'BoardTime', name: '板载时间', dataType: 'string' },
	{ identifier: 'BoardUID', name: '设备标识', dataType: 'string' },
	// 报警位图：bit0 高温 / bit1 高压 / bit2 过流 / bit3 欠压
	{
		identifier: 'alarmFlag', name: '报警标志', dataType: 'bitMap',
		bits: [
			{ bit: 0, name: '高温报警' },
			{ bit: 1, name: '高压报警' },
			{ bit: 2, name: '过流报警' },
			{ bit: 3, name: '欠压报警' }
		]
	},
	{ identifier: 'current', name: '电流', dataType: 'float', icon: '流', unit: 'A', decimals: 2, color: '#9775fa', metric: true, chart: true },
	// 阈值类（可设置）；如固件单位不同（如 mV），改这里的 unit 即可
	{ identifier: 'currentThresholdMax', name: '电流阈值上限', dataType: 'int32', unit: 'A', decimals: 0, setting: true },
	{ identifier: 'envTemp', name: '环境温度', dataType: 'float', icon: '环', unit: '°C', decimals: 1, color: '#4dabf7', metric: true, chart: true },
	// 风扇转速为百分比 0-100
	{ identifier: 'fanSpeed', name: '风扇转速', dataType: 'int32', icon: '风', unit: '%', decimals: 0, color: '#38d9a9', metric: true, scale: [0, 100] },
	// MSH 指令通道：App 把命令字符串写入该字符串属性，设备侧解析执行
	{ identifier: 'msh', name: 'MSH指令', dataType: 'string', setting: true },
	// 设备 MQTT 数据上报间隔（单位按固件约定，暂按秒；步进 5）
	{ identifier: 'mqttRefreshInterval', name: '上报间隔', dataType: 'int32', unit: 's', decimals: 0, step: 5, setting: true },
	{ identifier: 'power', name: '功率', dataType: 'float', icon: '率', unit: 'W', decimals: 1, color: '#f783ac', metric: true, chart: true },
	// 位图：bit0 继电器1，bit1 继电器2
	{
		identifier: 'relay', name: '继电器状态', dataType: 'bitMap', icon: '继',
		bits: [
			{ bit: 0, name: '继电器1' },
			{ bit: 1, name: '继电器2' }
		]
	},
	// 枚举：0=自动 1=手动，如固件定义不同改 enumMap 即可
	{
		identifier: 'tempControlMode', name: '温控模式', dataType: 'enum', setting: true,
		enumMap: { 0: '自动', 1: '手动' }
	},
	{ identifier: 'tempThresholdMax', name: '温度阈值上限', dataType: 'int32', unit: '°C', decimals: 0, setting: true },
	{ identifier: 'tempHysteresis', name: '温度回差', dataType: 'int32', unit: '°C', decimals: 0, setting: true },
	{ identifier: 'voltage', name: '电压', dataType: 'float', icon: '压', unit: 'V', decimals: 1, color: '#ffa94d', metric: true, chart: true },
	{ identifier: 'voltageThresholdMax', name: '电压阈值上限', dataType: 'int32', unit: 'V', decimals: 0, setting: true },
	{ identifier: 'voltageThresholdMin', name: '电压阈值下限', dataType: 'int32', unit: 'V', decimals: 0, setting: true }
]

export const ALL_IDENTIFIERS = PROP_META.map(p => p.identifier)
export const CHART_METRICS = PROP_META.filter(p => p.chart)

// 进入趋势图的属性
const CHART_KEYS = CHART_METRICS.map(m => m.identifier)
// 趋势图保留点数（历史接口单次最多 100 条）
const MAX_POINTS = 100
// 启动时回填历史的时间窗口（毫秒）
const HISTORY_WINDOW = 30 * 60 * 1000
// 超过该时长没有上报则判定为离线
const ONLINE_GAP = 5 * 60 * 1000

function emptyProps() {
	return {
		BoardBOOT: null,
		BoardTemp: null,
		BoardTime: '',
		BoardUID: '',
		alarmFlag: null,
		current: null,
		currentThresholdMax: null,
		envTemp: null,
		fanSpeed: null,
		msh: '',
		mqttRefreshInterval: null,
		power: null,
		relay: null,
		tempControlMode: null,
		tempThresholdMax: null,
		tempHysteresis: null,
		voltage: null,
		voltageThresholdMax: null,
		voltageThresholdMin: null
	}
}

export const state = reactive({
	online: null,
	propsData: emptyProps(),
	reportTime: '',
	lastReportTs: 0,
	// 采样点：{ t: 设备上报时间戳(ms), v: 值 }
	samples: {
		BoardTemp: [],
		envTemp: [],
		current: [],
		voltage: [],
		power: []
	},
	loading: false,
	refreshing: false,
	setting: false,
	// 历史记录是否已完成首次回填
	historyLoaded: false,
	lastError: ''
})

let timer = null

/**
 * map: { identifier: { value, time } }
 */
function applyData(map) {
	let latest = state.lastReportTs
	ALL_IDENTIFIERS.forEach(id => {
		if (map[id] && map[id].value !== '' && map[id].value !== undefined) {
			state.propsData[id] = map[id].value
		}
		if (map[id] && map[id].time > latest) {
			latest = map[id].time
		}
	})
	if (latest > 0) {
		state.lastReportTs = latest
		state.reportTime = formatTs(latest)
		state.online = Date.now() - latest < ONLINE_GAP
		// 按设备上报时间落分钟采样点
		CHART_KEYS.forEach(id => recordSample(id, latest))
	}
}

/**
 * 每次上报落一个点；同一时间戳的重复上报只更新该点的值
 */
function recordSample(id, ts) {
	const v = state.propsData[id]
	if (v === null || v === undefined || v === '') return
	const arr = state.samples[id]
	const last = arr[arr.length - 1]
	if (!last || last.t !== ts) {
		arr.push({ t: ts, v: Number(v) })
		if (arr.length > MAX_POINTS) arr.shift()
	} else {
		last.v = Number(v)
	}
}

/**
 * 1422：App 启动时拉取一次各趋势指标的历史记录回填图表，
 * 与后续实时轮询的采样点按上报时间戳合并去重
 */
function loadHistory() {
	const end = Date.now()
	const start = end - HISTORY_WINDOW
	return Promise.all(CHART_KEYS.map(id => {
		return queryPropertyHistory(id, start, end, MAX_POINTS).then(list => {
			const map = {}
			// 已有实时点优先（同一时间戳保留更新的值）
			state.samples[id].forEach(s => { map[s.t] = s.v })
			list.forEach(it => {
				const v = Number(it.value)
				if (!isNaN(v) && map[it.time] === undefined) {
					map[it.time] = v
				}
			})
			state.samples[id] = Object.keys(map)
				.map(t => ({ t: Number(t), v: map[t] }))
				.sort((a, b) => a.t - b.t)
				.slice(-MAX_POINTS)
		}).catch(err => {
			// 单个指标历史失败不影响其他指标
			console.warn('[OneNet] query-device-property-history(' + id + ') 失败', err)
		})
	})).then(() => {
		state.historyLoaded = true
	})
}

// 1421：平台缓存最新数据
function fetchLatest() {
	if (state.loading) return Promise.resolve()
	state.loading = true
	return queryLatestProperties().then(map => {
		applyData(map)
		state.lastError = ''
	}).catch(err => {
		console.error('[OneNet] query-device-property 失败', err)
		state.lastError = err.msg || '获取数据失败'
	}).finally(() => {
		state.loading = false
	})
}

const store = {
	state,

	start() {
		if (timer) return
		fetchLatest()
		// 首次启动回填一次历史趋势，之后只靠实时轮询追加
		if (!state.historyLoaded) {
			loadHistory()
		}
		timer = setInterval(fetchLatest, ONENET_CONFIG.POLL_INTERVAL)
	},

	stop() {
		if (timer) {
			clearInterval(timer)
			timer = null
		}
	},

	/**
	 * 1419：实时向设备查询；设备不在线/超时则回退 1421 缓存
	 * @returns Promise<'detail'|'cache'>
	 */
	refreshFromDevice() {
		if (state.refreshing) return Promise.resolve('skip')
		state.refreshing = true
		return queryPropertyDetail(ALL_IDENTIFIERS).then(valueMap => {
			const map = {}
			Object.keys(valueMap).forEach(k => {
				map[k] = { value: valueMap[k], time: Date.now() }
			})
			applyData(map)
			return 'detail'
		}).catch(err => {
			console.warn('[OneNet] query-device-property-detail 失败，回退最新缓存', err)
			return fetchLatest().then(() => 'cache')
		}).finally(() => {
			state.refreshing = false
		})
	},

	/**
	 * 1418：下发风扇转速（0-100）
	 */
	setFan(value) {
		state.setting = true
		return setDeviceProperty({ fanSpeed: value }).then(() => {
			state.propsData.fanSpeed = value
		}).finally(() => {
			state.setting = false
		})
	},

	/**
	 * 1418：下发继电器位图（bit0 继电器1 / bit1 继电器2，两个状态合并一次下发）
	 * @param r1 boolean 继电器1
	 * @param r2 boolean 继电器2
	 */
	setRelay(r1, r2) {
		const bitmap = (r1 ? 1 : 0) | (r2 ? 2 : 0)
		state.setting = true
		return setDeviceProperty({ relay: bitmap }).then(() => {
			state.propsData.relay = bitmap
		}).finally(() => {
			state.setting = false
		})
	},

	/**
	 * 1418：通用单个属性下发（温控模式 / 各阈值）
	 */
	setProperty(identifier, value) {
		const params = {}
		params[identifier] = value
		state.setting = true
		return setDeviceProperty(params).then(() => {
			state.propsData[identifier] = value
		}).finally(() => {
			state.setting = false
		})
	},

	/**
	 * 1418：下发 MSH 指令（写入字符串属性 msh）
	 * @param cmd string 完整命令，如 "reboot"、"set_ip 192.168.1.10:8080"
	 */
	sendMsh(cmd) {
		state.setting = true
		return setDeviceProperty({ msh: cmd }).then(() => {
			state.propsData.msh = cmd
		}).finally(() => {
			state.setting = false
		})
	}
}

// 通用位图读取：某 bit 是否为 1（null 表示设备尚未上报）
export function bitmapBitOn(bitmap, bit) {
	if (bitmap === null || bitmap === undefined || bitmap === '') return null
	return (Number(bitmap) & (1 << bit)) !== 0
}

// 兼容旧引用
export function relayBitOn(bitmap, bit) {
	return bitmapBitOn(bitmap, bit)
}

// 取当前触发的报警名称列表（按 bit 升序），无报警返回空数组
export function activeAlarms(bitmap) {
	const meta = getMeta('alarmFlag')
	if (!meta || bitmap === null || bitmap === undefined || bitmap === '') return []
	return meta.bits
		.filter(b => bitmapBitOn(bitmap, b.bit))
		.map(b => b.name)
}

export function getMeta(identifier) {
	return PROP_META.find(p => p.identifier === identifier)
}

export function formatNum(v, decimals) {
	if (v === undefined || v === null || v === '') return '--'
	return Number(v).toFixed(decimals)
}

export function formatTs(ts) {
	const d = new Date(Number(ts))
	const pad = n => (n < 10 ? '0' + n : '' + n)
	return d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()) +
		' ' + pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds())
}

// 趋势图横轴用 HH:MM
export function formatHM(ts) {
	const d = new Date(Number(ts))
	const pad = n => (n < 10 ? '0' + n : '' + n)
	return pad(d.getHours()) + ':' + pad(d.getMinutes())
}

// 趋势图横轴用 HH:MM:SS（5 秒级采样点）
export function formatHMS(ts) {
	const d = new Date(Number(ts))
	const pad = n => (n < 10 ? '0' + n : '' + n)
	return pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds())
}

export default store
