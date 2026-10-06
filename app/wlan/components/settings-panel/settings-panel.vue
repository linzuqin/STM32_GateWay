<template>
	<view class="page">
		<!-- 风扇控制卡片 -->
		<view class="card ctrl-card">
			<view class="fan-head">
				<view class="fan-head-left">
					<text class="fan-title">风扇转速设置</text>
					<text class="fan-sub">fanSpeed · int32 · 范围 0-100%</text>
				</view>
				<view class="status-pill" :class="onlineClass">
					<view class="status-dot"></view>
					<text class="status-text">{{ onlineText }}</text>
				</view>
			</view>

			<!-- 当前转速 -->
			<view class="fan-now-card">
				<view class="fan-ring" :style="{ background: ringStyle }">
					<view class="fan-ring-inner">
						<text class="fan-now-value">{{ currentSpeed }}</text>
						<text class="fan-now-percent">%</text>
					</view>
				</view>
				<text class="fan-now-label">设备当前转速</text>
			</view>

			<!-- 待下发值 -->
			<view class="draft-row">
				<text class="draft-label">设置值</text>
				<view class="draft-right">
					<text class="draft-value" :class="{ changed: isChanged }">{{ draftValue }}</text>
					<text class="draft-unit">%</text>
				</view>
			</view>
			<slider
				class="fan-slider"
				:value="draftValue"
				:min="0"
				:max="100"
				:step="1"
				activeColor="#38d9a9"
				backgroundColor="#e5e7eb"
				block-size="24"
				@changing="onChanging"
				@change="onChange"
			/>
			<view class="fan-scale">
				<text class="scale-text">0</text>
				<text class="scale-text">25</text>
				<text class="scale-text">50</text>
				<text class="scale-text">75</text>
				<text class="scale-text">100</text>
			</view>

			<!-- 快捷档位 -->
			<view class="quick-row">
				<view
					v-for="g in gears"
					:key="g.value"
					class="gear"
					:class="{ active: draftValue === g.value, pulse: draftValue === g.value && justSet === g.value }"
					@click="setDraft(g.value)"
				>
					<text class="gear-text" :class="{ active: draftValue === g.value }">{{ g.label }}</text>
				</view>
			</view>

			<view
				class="fan-btn"
				:class="{ disabled: cannotSubmit }"
				hover-class="btn-hover"
				@click="submit"
			>
				<text class="fan-btn-text">{{ state.setting ? '下发中…' : '下发设置' }}</text>
			</view>
			<text v-if="isChanged" class="ctrl-tip">未下发：当前 {{ currentSpeed }}% → 设置 {{ draftValue }}%</text>
			<text v-else-if="state.online === false" class="ctrl-tip warn">设备当前离线，设置需设备上线后执行</text>
			<text v-else class="ctrl-tip">设置值与设备当前转速一致</text>
		</view>

		<!-- 继电器控制卡片 -->
		<view class="card relay-card">
			<view class="relay-head">
				<view class="relay-head-left">
					<text class="relay-title">继电器控制</text>
					<text class="relay-sub">relay · bitMap · bit0 继电器1 / bit1 继电器2</text>
				</view>
			</view>

			<view class="relay-row">
				<view
					v-for="r in relays"
					:key="r.bit"
					class="relay-item"
					:class="{ on: r.draft }"
					@click="toggleRelay(r)"
				>
					<view class="relay-item-top">
						<text class="relay-name">{{ r.name }}</text>
						<view class="relay-switch" :class="{ on: r.draft }">
							<view class="relay-knob"></view>
						</view>
					</view>
					<text class="relay-state">{{ r.draft ? '闭合 (ON)' : '断开 (OFF)' }}</text>
					<text class="relay-current" v-if="r.current !== null">
						设备当前：{{ r.current ? 'ON' : 'OFF' }}
					</text>
					<text class="relay-current unknown" v-else>设备当前：--</text>
				</view>
			</view>

			<view class="relay-preview">
				<text class="preview-label">下发位图值</text>
				<text class="preview-value">relay = {{ relayBitmap }}（二进制 0b{{ relayBitmapBin }}）</text>
			</view>

			<view
				class="relay-btn"
				:class="{ disabled: relayCannotSubmit }"
				hover-class="btn-hover"
				@click="submitRelay"
			>
				<text class="relay-btn-text">{{ state.setting ? '下发中…' : '下发继电器状态' }}</text>
			</view>
			<text v-if="relayChanged" class="ctrl-tip">未下发：继电器1 {{ relays[0].draft ? 'ON' : 'OFF' }} · 继电器2 {{ relays[1].draft ? 'ON' : 'OFF' }}</text>
			<text v-else-if="state.online === false" class="ctrl-tip warn">设备当前离线，设置需设备上线后执行</text>
			<text v-else class="ctrl-tip">开关状态与设备一致，两路状态将合并一次下发</text>
		</view>

		<!-- 温控模式与阈值 -->
		<view class="card mode-card">
			<view class="mode-head">
				<text class="mode-title">运行参数与保护阈值</text>
				<text class="mode-sub">上报间隔 · 温控模式 · 电压/温度/电流阈值</text>
			</view>

			<!-- 温控模式切换 -->
			<view class="mode-line">
				<view class="mode-line-left">
					<text class="mode-label">温控模式</text>
					<text class="mode-current">
						设备当前：{{ modeCurrentText }}
					</text>
				</view>
				<view class="mode-seg">
					<view
						v-for="opt in modeOptions"
						:key="opt.value"
						class="mode-opt"
						:class="{ active: modeDraft === opt.value }"
						@click="setMode(opt.value)"
					>
						<text class="mode-opt-text" :class="{ active: modeDraft === opt.value }">{{ opt.label }}</text>
					</view>
				</view>
			</view>
			<view
				class="mode-send-btn"
				:class="{ disabled: modeCannotSubmit }"
				hover-class="btn-hover"
				@click="submitMode"
			>
				<text class="mode-send-text">{{ state.setting ? '下发中…' : '下发温控模式' }}</text>
			</view>

			<!-- 四个阈值 -->
			<view class="th-divider"></view>
			<view v-for="t in thresholdMetas" :key="t.identifier" class="th-row">
				<view class="th-info">
					<text class="th-name">{{ t.name }}</text>
					<text class="th-current">
						设备当前：{{ currentText(t.identifier) }} {{ t.unit }}
					</text>
				</view>
				<view class="th-ctrl">
					<view class="th-step" @click="stepThreshold(t, -1)"><text class="th-step-text">−</text></view>
					<input
						class="th-input"
						type="number"
						:value="thDraft(t.identifier)"
						@input="onThresholdInput(t.identifier, $event)"
					/>
					<view class="th-step" @click="stepThreshold(t, 1)"><text class="th-step-text">＋</text></view>
					<text class="th-unit">{{ t.unit }}</text>
					<view
						class="th-btn"
						:class="{ disabled: !thresholdChanged(t.identifier) || state.setting || state.online === false }"
						hover-class="btn-hover"
						@click="submitThreshold(t)"
					>
						<text class="th-btn-text">下发</text>
					</view>
				</view>
			</view>
			<text v-if="state.online === false" class="ctrl-tip warn">设备当前离线，设置需设备上线后执行</text>
		</view>

		<view class="footer-tip">
			<text>通过 set-device-property 接口下发（OneNet doc/1418）</text>
		</view>
	</view>
</template>

<script>
	import store, { state, relayBitOn, getMeta } from '@/utils/onenet/store.js'

	export default {
		name: 'SettingsPanel',
		data() {
			return {
				store,
				state,
				draftValue: 0,
				inited: false,
				justSet: -1,
				// 两路继电器的待下发开关（true=闭合 ON）
				draftRelay: [false, false],
				relayInited: false,
				// 温控模式待下发值
				modeDraft: null,
				modeInited: false,
				modeOptions: [
					{ value: 0, label: '自动' },
					{ value: 1, label: '手动' }
				],
				// 各阈值待下发值（字符串，便于 input 编辑）
				thresholdDrafts: {},
				thInited: {},
				gears: [
					{ label: '停止', value: 0 },
					{ label: '低档', value: 25 },
					{ label: '中档', value: 50 },
					{ label: '高档', value: 75 },
					{ label: '全速', value: 100 }
				]
			}
		},
		computed: {
			currentSpeed() {
				return state.propsData.fanSpeed === null ? '--' : state.propsData.fanSpeed
			},
			isChanged() {
				return state.propsData.fanSpeed !== null && this.draftValue !== state.propsData.fanSpeed
			},
			cannotSubmit() {
				return state.setting || state.online === false || state.propsData.fanSpeed === null
			},
			onlineClass() {
				return state.online === null ? 'unknown' : (state.online ? 'on' : 'off')
			},
			onlineText() {
				return state.online === null ? '未知' : (state.online ? '在线' : '离线')
			},
			ringStyle() {
				const pct = state.propsData.fanSpeed === null ? 0 : state.propsData.fanSpeed
				return 'conic-gradient(#38d9a9 ' + (pct * 3.6) + 'deg, #e5e7eb 0deg)'
			},
			// 两路继电器（草稿状态 + 设备当前状态）
			relays() {
				return [
					{ bit: 0, name: '继电器1', draft: this.draftRelay[0], current: relayBitOn(state.propsData.relay, 0) },
					{ bit: 1, name: '继电器2', draft: this.draftRelay[1], current: relayBitOn(state.propsData.relay, 1) }
				]
			},
			relayBitmap() {
				return (this.draftRelay[0] ? 1 : 0) | (this.draftRelay[1] ? 2 : 0)
			},
			relayBitmapBin() {
				return this.relayBitmap.toString(2).padStart(2, '0')
			},
			relayChanged() {
				const cur = state.propsData.relay
				if (cur === null) return true
				return this.relayBitmap !== Number(cur)
			},
			relayCannotSubmit() {
				return state.setting || state.online === false
			},
			// 可设置的整数参数（阈值 / 回差 / 上报间隔）
			thresholdMetas() {
				// 直接从物模型元数据取，新增参数自动出现
				const ids = ['mqttRefreshInterval', 'tempThresholdMax', 'tempHysteresis', 'voltageThresholdMax', 'voltageThresholdMin', 'currentThresholdMax']
				return ids.map(id => getMeta(id)).filter(Boolean)
			},
			modeCurrentText() {
				const m = getMeta('tempControlMode')
				const v = state.propsData.tempControlMode
				if (v === null || v === undefined || v === '') return '--'
				return m.enumMap[v] !== undefined ? m.enumMap[v] : v
			},
			modeCannotSubmit() {
				return state.setting || state.online === false ||
					this.modeDraft === null ||
					Number(this.modeDraft) === Number(state.propsData.tempControlMode)
			}
		},
		watch: {
			'state.propsData.fanSpeed'(v) {
				if (!this.inited && v !== null) {
					this.draftValue = v
					this.inited = true
				}
			},
			'state.propsData.relay'(v) {
				// 首次拿到设备状态时同步草稿；用户已改动未下发时不覆盖
				if (!this.relayInited && v !== null && v !== '') {
					this.draftRelay = [relayBitOn(v, 0), relayBitOn(v, 1)]
					this.relayInited = true
				}
			},
			'state.propsData.tempControlMode'(v) {
				if (!this.modeInited && v !== null && v !== undefined && v !== '') {
					this.modeDraft = Number(v)
					this.modeInited = true
				}
			},
			// 阈值首次上报时初始化输入框（只初始化一次，之后不覆盖用户编辑）
			state: {
				deep: true,
				handler() {
					this.thresholdMetas.forEach(t => {
						const v = state.propsData[t.identifier]
						if (!this.thInited[t.identifier] && v !== null && v !== undefined && v !== '') {
							this.thresholdDrafts[t.identifier] = String(v)
							this.thInited[t.identifier] = true
						}
					})
				}
			}
		},
		methods: {
			onChanging(e) {
				this.draftValue = e.detail.value
			},
			onChange(e) {
				this.draftValue = e.detail.value
			},
			setDraft(v) {
				this.draftValue = v
			},
			submit() {
				if (this.cannotSubmit) {
					if (state.online === false) {
						uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					}
					return
				}
				store.setFan(this.draftValue).then(() => {
					this.inited = true
					this.justSet = this.draftValue
					setTimeout(() => { this.justSet = -1 }, 500)
					uni.showToast({ title: '风扇设置已下发：' + this.draftValue + '%', icon: 'none' })
				}).catch(err => {
					console.error('[OneNet] set-device-property 失败', err)
					uni.showToast({ title: err.msg || '设置失败', icon: 'none' })
				})
			},
			toggleRelay(r) {
				// 直接改数组下标不触发响应式，用新数组替换
				const next = this.draftRelay.slice()
				next[r.bit] = !next[r.bit]
				this.draftRelay = next
			},
			submitRelay() {
				if (this.relayCannotSubmit) {
					if (state.online === false) {
						uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					}
					return
				}
				const r1 = this.draftRelay[0]
				const r2 = this.draftRelay[1]
				store.setRelay(r1, r2).then(() => {
					this.relayInited = true
					uni.showToast({
						title: '继电器已下发：' + (r1 ? '1ON' : '1OFF') + ' · ' + (r2 ? '2ON' : '2OFF'),
						icon: 'none'
					})
				}).catch(err => {
					console.error('[OneNet] set-device-property(relay) 失败', err)
					uni.showToast({ title: err.msg || '设置失败', icon: 'none' })
				})
			},
			// ===== 温控模式 =====
			setMode(v) {
				this.modeDraft = v
			},
			submitMode() {
				if (this.modeCannotSubmit) {
					if (state.online === false) uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					return
				}
				const label = this.modeOptions.find(o => o.value === this.modeDraft)
				store.setProperty('tempControlMode', this.modeDraft).then(() => {
					this.modeInited = true
					uni.showToast({ title: '温控模式已下发：' + (label ? label.label : this.modeDraft), icon: 'none' })
				}).catch(err => {
					console.error('[OneNet] set-device-property(tempControlMode) 失败', err)
					uni.showToast({ title: err.msg || '设置失败', icon: 'none' })
				})
			},
			// ===== 阈值 =====
			currentText(id) {
				const v = state.propsData[id]
				return v === null || v === undefined || v === '' ? '--' : v
			},
			thDraft(id) {
				const d = this.thresholdDrafts[id]
				return d === undefined ? '' : d
			},
			onThresholdInput(id, e) {
				this.thresholdDrafts[id] = e.detail.value
			},
			stepThreshold(t, delta) {
				const cur = parseInt(this.thresholdDrafts[t.identifier], 10)
				// 草稿为空时以设备当前值为基准（如上报间隔当前 5s，点 − 直接从 5 起算）
				const devCur = parseInt(state.propsData[t.identifier], 10)
				const base = isNaN(cur) ? (isNaN(devCur) ? 0 : devCur) : cur
				const step = t.step || 1
				const next = Math.max(0, base + delta * step)
				this.thresholdDrafts[t.identifier] = String(next)
			},
			thresholdChanged(id) {
				const cur = state.propsData[id]
				const d = this.thresholdDrafts[id]
				if (d === undefined || d === '') return false
				if (cur === null || cur === undefined || cur === '') return true
				return parseInt(d, 10) !== Number(cur)
			},
			submitThreshold(t) {
				if (state.setting || state.online === false) {
					if (state.online === false) uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					return
				}
				const val = parseInt(this.thresholdDrafts[t.identifier], 10)
				if (isNaN(val) || val < 0) {
					uni.showToast({ title: '请输入有效阈值', icon: 'none' })
					return
				}
				// 简单合理性校验：下限不能大于等于上限
				if (t.identifier === 'voltageThresholdMin' &&
					state.propsData.voltageThresholdMax !== null &&
					val >= Number(state.propsData.voltageThresholdMax)) {
					uni.showToast({ title: '下限需小于电压上限', icon: 'none' })
					return
				}
				store.setProperty(t.identifier, val).then(() => {
					this.thInited[t.identifier] = true
					uni.showToast({ title: t.name + '已下发：' + val + t.unit, icon: 'none' })
				}).catch(err => {
					console.error('[OneNet] set-device-property(' + t.identifier + ') 失败', err)
					uni.showToast({ title: err.msg || '设置失败', icon: 'none' })
				})
			}
		}
	}
</script>

<style>
	.ctrl-card {
		padding: 32rpx 28rpx 28rpx;
		margin-top: 8rpx;
	}

	.fan-head {
		display: flex;
		justify-content: space-between;
		align-items: flex-start;
	}

	.fan-head-left {
		display: flex;
		flex-direction: column;
	}

	.fan-title {
		font-size: 32rpx;
		font-weight: 600;
		color: #1f2937;
	}

	.fan-sub {
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 8rpx;
	}

	.status-pill {
		display: flex;
		flex-direction: row;
		align-items: center;
		border-radius: 100rpx;
		padding: 4rpx 16rpx;
		background: #f3f4f6;
	}

	.status-pill .status-dot {
		width: 12rpx;
		height: 12rpx;
		border-radius: 50%;
		margin-right: 8rpx;
		background: #9ca3af;
	}

	.status-pill.on {
		background: #dcfce7;
	}

	.status-pill.on .status-dot {
		background: #22c55e;
	}

	.status-pill.off {
		background: #fee2e2;
	}

	.status-pill.off .status-dot {
		background: #ef4444;
	}

	.status-text {
		font-size: 22rpx;
		color: #4b5563;
	}

	.status-pill.on .status-text {
		color: #16a34a;
	}

	.status-pill.off .status-text {
		color: #dc2626;
	}

	/* 当前转速环形展示 */
	.fan-now-card {
		display: flex;
		flex-direction: column;
		align-items: center;
		margin: 36rpx 0 20rpx;
	}

	.fan-ring {
		width: 220rpx;
		height: 220rpx;
		border-radius: 50%;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: background 0.6s ease;
	}

	.fan-ring-inner {
		width: 184rpx;
		height: 184rpx;
		border-radius: 50%;
		background: #ffffff;
		display: flex;
		flex-direction: row;
		align-items: baseline;
		justify-content: center;
	}

	.fan-now-value {
		font-size: 64rpx;
		font-weight: 700;
		color: #0f9f7e;
		line-height: 184rpx;
	}

	.fan-now-percent {
		font-size: 26rpx;
		color: #9ca3af;
	}

	.fan-now-label {
		font-size: 24rpx;
		color: #6b7280;
		margin-top: 16rpx;
	}

	/* 待下发值 */
	.draft-row {
		display: flex;
		justify-content: space-between;
		align-items: center;
		margin-top: 12rpx;
	}

	.draft-label {
		font-size: 26rpx;
		color: #6b7280;
	}

	.draft-right {
		display: flex;
		flex-direction: row;
		align-items: baseline;
	}

	.draft-value {
		font-size: 40rpx;
		font-weight: 600;
		color: #374151;
	}

	.draft-value.changed {
		color: #f59e0b;
	}

	.draft-unit {
		font-size: 22rpx;
		color: #9ca3af;
		margin-left: 6rpx;
	}

	.fan-slider {
		width: 100%;
		margin: 12rpx 0 0;
	}

	.fan-scale {
		display: flex;
		justify-content: space-between;
		padding: 0 10rpx;
	}

	.scale-text {
		font-size: 20rpx;
		color: #b0b6bf;
	}

	.quick-row {
		display: flex;
		justify-content: space-between;
		margin-top: 28rpx;
	}

	.gear {
		width: 18.5%;
		height: 64rpx;
		border-radius: 12rpx;
		background: #f3f4f6;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: all 0.25s ease;
	}

	.gear.active {
		background: #d3f8ec;
		border: 2rpx solid #38d9a9;
		transform: translateY(-4rpx);
	}

	.gear.pulse {
		animation: gear-pop 0.5s cubic-bezier(0.34, 1.56, 0.64, 1);
	}

	@keyframes gear-pop {
		0% { transform: scale(1); }
		40% { transform: scale(1.12); }
		100% { transform: translateY(-4rpx) scale(1); }
	}

	.gear-text {
		font-size: 24rpx;
		color: #6b7280;
	}

	.gear-text.active {
		color: #0f9f7e;
		font-weight: 600;
	}

	.fan-btn {
		margin-top: 32rpx;
		height: 84rpx;
		border-radius: 14rpx;
		background: linear-gradient(135deg, #20c997 0%, #38d9a9 100%);
		display: flex;
		align-items: center;
		justify-content: center;
		box-shadow: 0 8rpx 20rpx rgba(56, 217, 169, 0.3);
		transition: transform 0.15s ease, opacity 0.2s;
	}

	.fan-btn:active {
		transform: scale(0.97);
	}

	.fan-btn.disabled {
		opacity: 0.5;
		box-shadow: none;
	}

	.btn-hover {
		opacity: 0.85;
	}

	.fan-btn-text {
		font-size: 28rpx;
		color: #ffffff;
		font-weight: 500;
	}

	.ctrl-tip {
		display: block;
		text-align: center;
		font-size: 22rpx;
		color: #9ca3af;
		margin-top: 16rpx;
	}

	.ctrl-tip.warn {
		color: #f59e0b;
	}

	/* 继电器控制 */
	.relay-card {
		padding: 32rpx 28rpx 28rpx;
		margin-top: 24rpx;
		animation: card-in 0.45s 0.1s ease both;
	}

	@keyframes card-in {
		from { opacity: 0; transform: translateY(20rpx); }
		to { opacity: 1; transform: translateY(0); }
	}

	.relay-head {
		display: flex;
		justify-content: space-between;
		align-items: flex-start;
	}

	.relay-title {
		font-size: 32rpx;
		font-weight: 600;
		color: #1f2937;
	}

	.relay-sub {
		display: block;
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 8rpx;
	}

	.relay-row {
		display: flex;
		justify-content: space-between;
		margin-top: 28rpx;
	}

	.relay-item {
		width: 48%;
		box-sizing: border-box;
		border-radius: 16rpx;
		border: 2rpx solid #e5e7eb;
		background: #f9fafb;
		padding: 24rpx 22rpx;
		transition: all 0.25s ease;
	}

	.relay-item.on {
		border-color: #2b7fff;
		background: #eff6ff;
		box-shadow: 0 8rpx 20rpx rgba(43, 127, 255, 0.16);
	}

	.relay-item-top {
		display: flex;
		justify-content: space-between;
		align-items: center;
	}

	.relay-name {
		font-size: 28rpx;
		font-weight: 600;
		color: #374151;
	}

	.relay-item.on .relay-name {
		color: #2b7fff;
	}

	/* 开关 */
	.relay-switch {
		width: 84rpx;
		height: 46rpx;
		border-radius: 100rpx;
		background: #d1d5db;
		position: relative;
		transition: background 0.25s ease;
	}

	.relay-switch.on {
		background: #2b7fff;
	}

	.relay-knob {
		position: absolute;
		top: 5rpx;
		left: 5rpx;
		width: 36rpx;
		height: 36rpx;
		border-radius: 50%;
		background: #ffffff;
		box-shadow: 0 2rpx 6rpx rgba(0, 0, 0, 0.18);
		transition: transform 0.25s cubic-bezier(0.34, 1.4, 0.64, 1);
	}

	.relay-switch.on .relay-knob {
		transform: translateX(38rpx);
	}

	.relay-state {
		display: block;
		font-size: 26rpx;
		font-weight: 600;
		color: #9ca3af;
		margin-top: 18rpx;
	}

	.relay-item.on .relay-state {
		color: #2b7fff;
	}

	.relay-current {
		display: block;
		font-size: 20rpx;
		color: #6b7280;
		margin-top: 6rpx;
	}

	.relay-current.unknown {
		color: #b0b6bf;
	}

	.relay-preview {
		display: flex;
		justify-content: space-between;
		align-items: center;
		margin-top: 24rpx;
		padding: 16rpx 20rpx;
		border-radius: 12rpx;
		background: #f3f4f6;
	}

	.preview-label {
		font-size: 22rpx;
		color: #6b7280;
	}

	.preview-value {
		font-size: 24rpx;
		color: #374151;
		font-weight: 600;
		font-family: monospace;
	}

	.relay-btn {
		margin-top: 24rpx;
		height: 84rpx;
		border-radius: 14rpx;
		background: linear-gradient(135deg, #2b7fff 0%, #4fa3ff 100%);
		display: flex;
		align-items: center;
		justify-content: center;
		box-shadow: 0 8rpx 20rpx rgba(43, 127, 255, 0.28);
		transition: transform 0.15s ease, opacity 0.2s;
	}

	.relay-btn:active {
		transform: scale(0.97);
	}

	.relay-btn.disabled {
		opacity: 0.5;
		box-shadow: none;
	}

	.relay-btn-text {
		font-size: 28rpx;
		color: #ffffff;
		font-weight: 500;
	}

	/* 温控模式与阈值 */
	.mode-card {
		padding: 32rpx 28rpx 28rpx;
		margin-top: 24rpx;
		animation: card-in 0.45s 0.18s ease both;
	}

	.mode-head {
		display: flex;
		flex-direction: column;
	}

	.mode-title {
		font-size: 32rpx;
		font-weight: 600;
		color: #1f2937;
	}

	.mode-sub {
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 8rpx;
	}

	.mode-line {
		display: flex;
		justify-content: space-between;
		align-items: center;
		margin-top: 28rpx;
	}

	.mode-line-left {
		display: flex;
		flex-direction: column;
	}

	.mode-label {
		font-size: 28rpx;
		font-weight: 600;
		color: #374151;
	}

	.mode-current {
		font-size: 21rpx;
		color: #9ca3af;
		margin-top: 6rpx;
	}

	.mode-seg {
		display: flex;
		background: #f3f4f6;
		border-radius: 12rpx;
		padding: 4rpx;
	}

	.mode-opt {
		padding: 12rpx 34rpx;
		border-radius: 10rpx;
		transition: all 0.25s ease;
	}

	.mode-opt.active {
		background: #2b7fff;
		box-shadow: 0 4rpx 12rpx rgba(43, 127, 255, 0.3);
	}

	.mode-opt-text {
		font-size: 25rpx;
		color: #6b7280;
	}

	.mode-opt-text.active {
		color: #ffffff;
		font-weight: 600;
	}

	.mode-send-btn {
		margin-top: 24rpx;
		height: 76rpx;
		border-radius: 14rpx;
		background: linear-gradient(135deg, #2b7fff 0%, #4fa3ff 100%);
		display: flex;
		align-items: center;
		justify-content: center;
		box-shadow: 0 8rpx 20rpx rgba(43, 127, 255, 0.28);
		transition: transform 0.15s ease, opacity 0.2s;
	}

	.mode-send-btn:active {
		transform: scale(0.97);
	}

	.mode-send-btn.disabled {
		opacity: 0.5;
		box-shadow: none;
	}

	.mode-send-text {
		font-size: 27rpx;
		color: #ffffff;
		font-weight: 500;
	}

	.th-divider {
		height: 2rpx;
		background: #f1f3f6;
		margin: 28rpx 0 8rpx;
	}

	.th-row {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding: 20rpx 0;
		border-bottom: 2rpx solid #f7f8fa;
	}

	.th-info {
		display: flex;
		flex-direction: column;
	}

	.th-name {
		font-size: 27rpx;
		color: #1f2937;
		font-weight: 500;
	}

	.th-current {
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 6rpx;
	}

	.th-ctrl {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.th-step {
		width: 52rpx;
		height: 52rpx;
		border-radius: 10rpx;
		background: #f3f4f6;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.th-step:active {
		background: #e5e7eb;
	}

	.th-step-text {
		font-size: 34rpx;
		color: #4b5563;
		line-height: 1;
	}

	.th-input {
		width: 96rpx;
		height: 52rpx;
		margin: 0 10rpx;
		text-align: center;
		font-size: 28rpx;
		font-weight: 600;
		color: #111827;
		background: #f9fafb;
		border-radius: 10rpx;
	}

	.th-unit {
		font-size: 22rpx;
		color: #9ca3af;
		margin-right: 14rpx;
	}

	.th-btn {
		padding: 0 24rpx;
		height: 56rpx;
		border-radius: 10rpx;
		background: #2b7fff;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.th-btn.disabled {
		background: #c7d2e0;
	}

	.th-btn-text {
		font-size: 24rpx;
		color: #ffffff;
	}
</style>
