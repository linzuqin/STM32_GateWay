<template>
	<view class="page">
		<!-- 顶部设备状态条 -->
		<view class="topbar card">
			<view class="topbar-left">
				<text class="topbar-name">环境监测</text>
				<view class="status-pill" :class="onlineClass">
					<view class="status-dot"></view>
					<text class="status-text">{{ onlineText }}</text>
				</view>
			</view>
			<view class="refresh-btn" :class="{ spinning: state.refreshing }" @click="onRefresh">
				<text class="refresh-icon">⟳</text>
			</view>
		</view>

		<!-- 报警横幅：alarmFlag 任一位为 1 时显示 -->
		<view v-if="alarms.length" class="alarm-banner">
			<view class="alarm-bell">
				<text class="alarm-bell-text">!</text>
			</view>
			<view class="alarm-body">
				<text class="alarm-title">{{ alarms.length }} 项报警</text>
				<text class="alarm-names">{{ alarms.join(' · ') }}</text>
			</view>
		</view>

		<!-- 实时数据 -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">实时数据</text>
			<text class="title-extra">每 {{ pollSeconds }} 秒刷新</text>
		</view>
		<view class="metric-grid">
			<!-- 双温度卡：板载温度 / 环境温度挤在同一张半宽卡 -->
			<view class="metric-card card dual-temp-card">
				<view class="temp-half">
					<view class="temp-head">
						<view class="metric-icon sm" :style="{ background: tempPair[0].color }">
							<text class="icon-text">{{ tempPair[0].icon }}</text>
						</view>
						<text class="metric-label">{{ tempPair[0].name }}</text>
					</view>
					<view class="metric-value-row">
						<text class="temp-value">{{ valueOf(tempPair[0]) }}</text>
						<text class="metric-unit">{{ tempPair[0].unit }}</text>
					</view>
				</view>
				<view class="temp-divider"></view>
				<view class="temp-half">
					<view class="temp-head">
						<view class="metric-icon sm" :style="{ background: tempPair[1].color }">
							<text class="icon-text">{{ tempPair[1].icon }}</text>
						</view>
						<text class="metric-label">{{ tempPair[1].name }}</text>
					</view>
					<view class="metric-value-row">
						<text class="temp-value">{{ valueOf(tempPair[1]) }}</text>
						<text class="metric-unit">{{ tempPair[1].unit }}</text>
					</view>
				</view>
			</view>

			<view
				v-for="(m, mi) in otherMetrics"
				:key="m.identifier"
				class="metric-card card"
				:style="{ animationDelay: (mi + 1) * 0.06 + 's' }"
				@click="onCardTap(m)"
			>
				<view class="metric-icon" :style="{ background: m.color }">
					<text class="icon-text">{{ m.icon }}</text>
				</view>
				<view class="metric-body">
					<text class="metric-label">{{ m.name }}</text>
					<view class="metric-value-row">
						<text class="metric-value">{{ valueOf(m) }}</text>
						<text class="metric-unit">{{ m.unit }}</text>
					</view>
				</view>
				<text v-if="m.identifier === 'fanSpeed'" class="metric-link">去设置 ›</text>
			</view>
		</view>

		<!-- 实时趋势（左右滑动切换） -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">实时趋势</text>
			<text class="title-extra">5 秒 1 点 · 左右滑动切换</text>
		</view>
		<view class="trend-card card">
			<view class="trend-header">
				<view class="trend-title">
					<text class="trend-name" :style="{ color: currentChart.color }">{{ currentChart.name }}</text>
					<text class="trend-desc">最近 {{ samples.length }} 个采样点</text>
				</view>
				<view class="trend-now">
					<text class="trend-value" :style="{ color: currentChart.color }">{{ valueOf(currentChart) }}</text>
					<text class="trend-unit">{{ currentChart.unit }}</text>
				</view>
			</view>

			<swiper
				class="trend-swiper"
				:current="chartIndex"
				:indicator-dots="false"
				:circular="false"
				@change="onSwiperChange"
			>
				<swiper-item v-for="(m, ci) in chartMetrics" :key="m.identifier">
					<view class="chart-wrap">
					<canvas
						:id="'cv' + ci"
						:canvas-id="'cv' + ci"
						class="trend-canvas"
						@touchstart="onChartTouch($event, ci)"
					></canvas>
					<!-- 透明触摸层：只接管点按，不监听 touchmove，避免拦截 swiper 横滑翻页 -->
					<view
						:id="'touch' + ci"
						class="chart-touch-layer"
						@touchstart="onChartTouch($event, ci)"
					></view>
					<view v-if="samplesOf(m.identifier).length === 0" class="chart-empty">
						<text>{{ state.historyLoaded ? '暂无历史数据，等待新的上报…' : '正在加载历史数据…' }}</text>
					</view>
				</view>
				</swiper-item>
			</swiper>

			<!-- 时间横轴：跨度大时只显示时:分 -->
			<view class="axis" v-if="samples.length === 1">
				<text class="axis-text">{{ axisTime(samples[0].t) }}</text>
			</view>
			<view class="axis three" v-else-if="samples.length > 1">
				<text class="axis-text">{{ axisTime(samples[0].t) }}</text>
				<text class="axis-text">{{ axisTime(samples[Math.floor(samples.length / 2)].t) }}</text>
				<text class="axis-text">{{ axisTime(samples[samples.length - 1].t) }}</text>
			</view>

			<!-- 自定义指示点 -->
			<text v-if="samples.length > 0 && hintPoint === null" class="chart-tap-hint">点按曲线查看数值</text>
			<view class="dots">
				<view
					v-for="(m, i) in chartMetrics"
					:key="m.identifier"
					class="dot"
					:class="{ active: i === chartIndex }"
					:style="i === chartIndex ? { background: m.color } : {}"
					@click="tapDot(i)"
				></view>
			</view>

			<view class="trend-range" v-if="samples.length >= 1">
				<text class="range-text">最低 {{ formatNum(range.min, currentChart.decimals) }}{{ currentChart.unit }}</text>
				<text class="range-text">平均 {{ formatNum(range.avg, currentChart.decimals) }}{{ currentChart.unit }}</text>
				<text class="range-text">最高 {{ formatNum(range.max, currentChart.decimals) }}{{ currentChart.unit }}</text>
			</view>
		</view>

		<view class="footer-tip">
			<text>OneNet 真实数据 · 产品 {{ productId }}</text>
		</view>
	</view>
</template>

<script>
	import store, { state, PROP_META, formatNum, formatHMS, activeAlarms } from '@/utils/onenet/store.js'
	import { ONENET_CONFIG } from '@/utils/onenet/config.js'

	export default {
		name: 'HomePanel',
		data() {
			return {
				store,
				state,
				productId: ONENET_CONFIG.PRODUCT_ID,
				pollSeconds: Math.round(ONENET_CONFIG.POLL_INTERVAL / 1000),
				chartIndex: 0,
				animTimer: null,
				sizeReady: false,
				// 触摸选中的数据点序号（仅对当前 chartIndex 有效）
				hintPoint: null
			}
		},
		created() {
			// 各 canvas 的布局信息缓存（触摸坐标换算用），非响应式
			this.rects = {}
		},
		computed: {
			// 当前触发的报警
			alarms() {
				return activeAlarms(state.propsData.alarmFlag)
			},
			// 双温度卡
			tempPair() {
				return [
					PROP_META.find(p => p.identifier === 'BoardTemp'),
					PROP_META.find(p => p.identifier === 'envTemp')
				]
			},
			// 除两个温度外的实时卡片
			otherMetrics() {
				return PROP_META.filter(p => p.metric &&
					p.identifier !== 'BoardTemp' && p.identifier !== 'envTemp')
			},
			chartMetrics() {
				return PROP_META.filter(p => p.chart)
			},
			currentChart() {
				return this.chartMetrics[this.chartIndex]
			},
			samples() {
				return state.samples[this.currentChart.identifier] || []
			},
			onlineClass() {
				return state.online === null ? 'unknown' : (state.online ? 'on' : 'off')
			},
			onlineText() {
				return state.online === null ? '未知' : (state.online ? '在线' : '离线')
			},
			range() {
				const arr = this.samples.map(s => s.v)
				if (!arr.length) return { min: 0, max: 0, avg: 0 }
				const min = Math.min.apply(null, arr)
				const max = Math.max.apply(null, arr)
				const avg = arr.reduce((a, b) => a + b, 0) / arr.length
				return { min, max, avg }
			},
			// 数据变化指纹：任一趋势序列新增/更新点都触发重绘
			chartTick() {
				return this.chartMetrics.map(m => {
					const arr = state.samples[m.identifier] || []
					const last = arr[arr.length - 1]
					return m.identifier + ':' + arr.length + ':' + (last ? last.t + '=' + last.v : '')
				}).join('|')
			}
		},
		watch: {
			chartTick() {
				this.$nextTick(() => this.drawChart(this.chartIndex, 1))
			}
		},
		mounted() {
			// 等待 swiper/canvas 完成布局后首绘
			setTimeout(() => {
				this.sizeReady = true
				this.animateChart(this.chartIndex)
			}, 120)
		},
		beforeUnmount() {
			this.clearAnim()
		},
		methods: {
			formatNum,
			formatHMS,
			// 跨度超过 2 分钟时横轴只显示 HH:MM，否则 HH:MM:SS
			axisTime(t) {
				const s = this.samples
				const span = s.length > 1 ? s[s.length - 1].t - s[0].t : 0
				if (span <= 2 * 60 * 1000) return formatHMS(t)
				const d = new Date(t)
				const p = n => (n < 10 ? '0' + n : '' + n)
				return p(d.getHours()) + ':' + p(d.getMinutes())
			},
			valueOf(m) {
				return formatNum(state.propsData[m.identifier], m.decimals)
			},
			samplesOf(key) {
				return state.samples[key] || []
			},
			tapDot(i) {
			if (i === this.chartIndex) return
			this.hintPoint = null
			this.chartIndex = i
			this.$nextTick(() => this.animateChart(i))
		},
		onSwiperChange(e) {
			const idx = e.detail.current
			if (idx === this.chartIndex) return
			this.hintPoint = null
			this.chartIndex = idx
			this.$nextTick(() => this.animateChart(idx))
		},
		// 点按图表：立即吸附最近的数据点；不监听 touchmove，横滑自然由 swiper 翻页处理
		// canvas 与触摸层都可能触发，这里做一次去重
		onChartTouch(e, idx) {
			const t = (e.touches && e.touches[0]) || (e.changedTouches && e.changedTouches[0])
			if (!t) return
			const x = typeof t.clientX === 'number' ? t.clientX : (typeof t.pageX === 'number' ? t.pageX : t.x)
			if (typeof x !== 'number') return

			// 同一次按下 canvas 与触摸层各派发一次，去重
			const now = Date.now()
			if (this._lastTapKey === idx && now - (this._lastTapTime || 0) < 80) return
			this._lastTapKey = idx
			this._lastTapTime = now

			uni.createSelectorQuery().in(this)
				.select('#touch' + idx)
				.boundingClientRect(rect => {
					rect = rect || this.rects[idx]
					if (!rect) return
					this.rects[idx] = rect
					const m = this.chartMetrics[idx]
					if (!m) return
					const all = state.samples[m.identifier] || []
					if (!all.length) return
					const padL = 40
					const iw = rect.width - padL - 14
					let f = (x - rect.left - padL) / iw
					f = Math.min(Math.max(f, 0), 1)
					const pi = all.length === 1 ? 0 : Math.round(f * (all.length - 1))
					this.chartIndex = idx
					if (pi !== this.hintPoint) {
						this.hintPoint = pi
						this.drawChart(idx, 1)
					}
				}).exec()
		},
			onCardTap(m) {
				if (m.chart) {
					const idx = this.chartMetrics.findIndex(c => c.identifier === m.identifier)
					if (idx >= 0) this.tapDot(idx)
				} else if (m.identifier === 'fanSpeed') {
					this.$emit('go-settings')
				}
			},
			clearAnim() {
				if (this.animTimer) {
					clearInterval(this.animTimer)
					this.animTimer = null
				}
			},
			// 切换指标时的描绘动画
		animateChart(idx) {
			this.clearAnim()
			this.hintPoint = null
			const start = Date.now()
			const dur = 480
			this.animTimer = setInterval(() => {
				const t = Math.min(1, (Date.now() - start) / dur)
				const eased = 1 - Math.pow(1 - t, 3)
				this.drawChart(idx, eased)
				if (t >= 1) this.clearAnim()
			}, 20)
		},
		// canvas 圆角矩形路径（旧版 canvas API 无 roundRect）
		roundRect(ctx, x, y, w, h, r) {
			const rr = Math.min(r, w / 2, h / 2)
			ctx.beginPath()
			ctx.moveTo(x + rr, y)
			ctx.lineTo(x + w - rr, y)
			ctx.arcTo(x + w, y, x + w, y + rr, rr)
			ctx.lineTo(x + w, y + h - rr)
			ctx.arcTo(x + w, y + h, x + w - rr, y + h, rr)
			ctx.lineTo(x + rr, y + h)
			ctx.arcTo(x, y + h, x, y + h - rr, rr)
			ctx.lineTo(x, y + rr)
			ctx.arcTo(x, y, x + rr, y, rr)
			ctx.closePath()
		},
		hexToRgba(hex, alpha) {
				const h = hex.replace('#', '')
				const r = parseInt(h.substring(0, 2), 16)
				const g = parseInt(h.substring(2, 4), 16)
				const b = parseInt(h.substring(4, 6), 16)
				return 'rgba(' + r + ',' + g + ',' + b + ',' + alpha + ')'
			},
			/**
			 * 用 canvas 绘制折线+面积趋势
			 * @param idx 图表序号（cv0/cv1）
			 * @param progress 0~1 描绘进度（数据点截取比例）
			 */
			drawChart(idx, progress) {
				const id = 'cv' + idx
				const m = this.chartMetrics[idx]
				if (!m) return
				const all = state.samples[m.identifier] || []

				const ctx = uni.createCanvasContext(id, this)
				uni.createSelectorQuery().in(this).select('#' + id).boundingClientRect(rect => {
				const W = (rect && rect.width) || 320
				const H = (rect && rect.height) || 200
				if (rect) this.rects[idx] = rect

				ctx.clearRect(0, 0, W, H)

					const padL = 40
					const padR = 14
					const padT = 30
					const padB = 12
					const iw = W - padL - padR
					const ih = H - padT - padB

					// 纵轴量程
					let min = 0
					let max = 1
					if (all.length) {
						const vals = all.map(s => s.v)
						min = Math.min.apply(null, vals)
						max = Math.max.apply(null, vals)
					}
					let span = max - min
					if (span === 0) span = Math.abs(max) * 0.2 || 1
					min -= span * 0.15
					max += span * 0.15
					span = max - min

					// 横向网格 + 纵轴刻度
					ctx.setLineWidth(1)
					for (let r = 0; r <= 4; r++) {
						const gy = padT + ih * r / 4
						ctx.beginPath()
						ctx.moveTo(padL, gy)
						ctx.lineTo(padL + iw, gy)
						ctx.setStrokeStyle('#eef1f5')
						ctx.stroke()
						const gv = max - span * r / 4
						ctx.setFontSize(9)
						ctx.setTextAlign('right')
						ctx.setFillStyle('#b0b6bf')
						ctx.fillText(this.formatNum(gv, m.decimals), padL - 6, gy + 3)
					}

					if (!all.length) {
						ctx.draw()
						return
					}

					// 按描绘进度截取点
					const count = Math.max(1, Math.ceil(all.length * progress))
					const pts = all.slice(0, count)
					const xOf = i => pts.length === 1
						? padL + iw / 2
						: padL + iw * i / (pts.length - 1)
					const yOf = v => padT + ih * (1 - (v - min) / span)

					// 面积填充
					const grad = ctx.createLinearGradient(0, padT, 0, padT + ih)
					grad.addColorStop(0, this.hexToRgba(m.color, 0.30))
					grad.addColorStop(1, this.hexToRgba(m.color, 0.02))
					ctx.beginPath()
					ctx.moveTo(xOf(0), yOf(pts[0].v))
					for (let i = 1; i < pts.length; i++) {
						ctx.lineTo(xOf(i), yOf(pts[i].v))
					}
					ctx.lineTo(xOf(pts.length - 1), padT + ih)
					ctx.lineTo(xOf(0), padT + ih)
					ctx.closePath()
					ctx.setFillStyle(grad)
					ctx.fill()

					// 折线
					ctx.beginPath()
					ctx.moveTo(xOf(0), yOf(pts[0].v))
					for (let i = 1; i < pts.length; i++) {
						ctx.lineTo(xOf(i), yOf(pts[i].v))
					}
					ctx.setStrokeStyle(m.color)
					ctx.setLineWidth(2.2)
					ctx.setLineCap('round')
					ctx.setLineJoin('round')
					ctx.stroke()

					// 数据点（只在点不太密时画小圆点，最后一个点始终高亮）
					if (all.length <= 40) {
						for (let i = 0; i < pts.length - 1; i++) {
							ctx.beginPath()
							ctx.arc(xOf(i), yOf(pts[i].v), 1.8, 0, Math.PI * 2)
							ctx.setFillStyle(this.hexToRgba(m.color, 0.55))
							ctx.fill()
						}
					}
					const lx = xOf(pts.length - 1)
					const ly = yOf(pts[pts.length - 1].v)
					ctx.beginPath()
					ctx.arc(lx, ly, 7, 0, Math.PI * 2)
					ctx.setFillStyle(this.hexToRgba(m.color, 0.18))
					ctx.fill()
					ctx.beginPath()
					ctx.arc(lx, ly, 3.6, 0, Math.PI * 2)
					ctx.setFillStyle(m.color)
					ctx.fill()

					// 最新值标签（触摸选中时隐藏，避免与气泡重叠）
				if (this.hintPoint === null || idx !== this.chartIndex || progress < 1) {
					ctx.setFontSize(10)
					ctx.setTextAlign('center')
					ctx.setFillStyle(m.color)
					const label = this.formatNum(pts[pts.length - 1].v, m.decimals) + m.unit
					ctx.fillText(label, Math.min(Math.max(lx, 34), W - 24), ly - 12)
				}

				// 触摸提示：竖线 + 吸附高亮点 + 数值/时间气泡
				if (progress >= 1 && idx === this.chartIndex && this.hintPoint !== null) {
					let hi = Math.min(Math.max(this.hintPoint, 0), all.length - 1)
					const n = all.length
					const hx = n === 1 ? padL + iw / 2 : padL + iw * hi / (n - 1)
					const hy = yOf(all[hi].v)

					ctx.beginPath()
					ctx.moveTo(hx, padT)
					ctx.lineTo(hx, padT + ih)
					ctx.setStrokeStyle(this.hexToRgba(m.color, 0.45))
					ctx.setLineWidth(1)
					ctx.stroke()

					// 吸附高亮点：白底彩圈
					ctx.beginPath()
					ctx.arc(hx, hy, 6.5, 0, Math.PI * 2)
					ctx.setFillStyle('#ffffff')
					ctx.fill()
					ctx.beginPath()
					ctx.arc(hx, hy, 6.5, 0, Math.PI * 2)
					ctx.setStrokeStyle(m.color)
					ctx.setLineWidth(2)
					ctx.stroke()
					ctx.beginPath()
					ctx.arc(hx, hy, 2.6, 0, Math.PI * 2)
					ctx.setFillStyle(m.color)
					ctx.fill()

					// 气泡
					const valTxt = this.formatNum(all[hi].v, m.decimals) + m.unit
					const timeTxt = formatHMS(all[hi].t)
					const bw = 108
					const bh = 42
					let bx = Math.min(Math.max(hx - bw / 2, padL), padL + iw - bw)
					let by = hy - bh - 12
					let below = false
					if (by < padT) {
						by = hy + 12
						below = true
						if (by + bh > padT + ih) by = padT + ih - bh - 2
					}
					ctx.beginPath()
					this.roundRect(ctx, bx, by, bw, bh, 10)
					ctx.setFillStyle('#ffffff')
					ctx.fill()
					ctx.setStrokeStyle(this.hexToRgba(m.color, 0.45))
					ctx.setLineWidth(1)
					ctx.stroke()

					// 小三角指向数据点
					ctx.beginPath()
					if (below) {
						ctx.moveTo(hx - 5, by)
						ctx.lineTo(hx + 5, by)
						ctx.lineTo(hx, by - 6)
					} else {
						ctx.moveTo(hx - 5, by + bh)
						ctx.lineTo(hx + 5, by + bh)
						ctx.lineTo(hx, by + bh + 6)
					}
					ctx.closePath()
					ctx.setFillStyle('#ffffff')
					ctx.fill()

					ctx.setTextAlign('center')
					ctx.setFontSize(12)
					ctx.setFillStyle(m.color)
					ctx.fillText(valTxt, bx + bw / 2, by + 18)
					ctx.setFontSize(9)
					ctx.setFillStyle('#8a9099')
					ctx.fillText(timeTxt, bx + bw / 2, by + 33)
				}

				ctx.draw()
			}).exec()
			},
			onRefresh() {
				store.refreshFromDevice().then(source => {
					if (source === 'skip') return
					uni.showToast({
						title: source === 'detail' ? '已刷新设备实时数据' : '设备未响应，已加载最新缓存',
						icon: 'none'
					})
				})
			}
		}
	}
</script>

<style>
	/* 顶部状态条 */
	.topbar {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding: 28rpx;
		animation: card-in 0.4s ease both;
	}

	@keyframes card-in {
		from { opacity: 0; transform: translateY(20rpx); }
		to { opacity: 1; transform: translateY(0); }
	}

	.topbar-left {
		display: flex;
		flex-direction: column;
	}

	.topbar-name {
		font-size: 34rpx;
		font-weight: 600;
		color: #1f2937;
	}

	.status-pill {
		display: flex;
		flex-direction: row;
		align-items: center;
		margin-top: 10rpx;
		border-radius: 100rpx;
		padding: 4rpx 16rpx;
		align-self: flex-start;
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
		animation: pulse 1.6s infinite;
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

	@keyframes pulse {
		0% { opacity: 1; }
		50% { opacity: 0.3; }
		100% { opacity: 1; }
	}

	.refresh-btn {
		width: 68rpx;
		height: 68rpx;
		border-radius: 50%;
		background: #eff6ff;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	/* 报警横幅 */
	.alarm-banner {
		display: flex;
		flex-direction: row;
		align-items: center;
		margin: 20rpx 24rpx 0;
		padding: 22rpx 24rpx;
		border-radius: 18rpx;
		background: linear-gradient(135deg, #fff1f2 0%, #ffe4e6 100%);
		border: 2rpx solid #fda4af;
		animation: alarm-in 0.4s ease both;
	}

	@keyframes alarm-in {
		from { opacity: 0; transform: translateY(-12rpx); }
		to { opacity: 1; transform: translateY(0); }
	}

	.alarm-bell {
		width: 56rpx;
		height: 56rpx;
		border-radius: 50%;
		background: #ef4444;
		display: flex;
		align-items: center;
		justify-content: center;
		margin-right: 18rpx;
		animation: alarm-blink 1s infinite;
	}

	@keyframes alarm-blink {
		0%, 100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.45); }
		50% { box-shadow: 0 0 0 12rpx rgba(239, 68, 68, 0); }
	}

	.alarm-bell-text {
		font-size: 36rpx;
		font-weight: 700;
		color: #ffffff;
		line-height: 1;
	}

	.alarm-body {
		display: flex;
		flex-direction: column;
		flex: 1;
	}

	.alarm-title {
		font-size: 28rpx;
		font-weight: 600;
		color: #dc2626;
	}

	.alarm-names {
		font-size: 23rpx;
		color: #b91c1c;
		margin-top: 6rpx;
	}

	.refresh-icon {
		font-size: 40rpx;
		color: #2b7fff;
		line-height: 1;
	}

	.refresh-btn.spinning .refresh-icon {
		animation: spin 0.6s linear;
	}

	@keyframes spin {
		from { transform: rotate(0deg); }
		to { transform: rotate(360deg); }
	}

	.title-extra {
		font-size: 20rpx;
		color: #9ca3af;
		margin-left: auto;
		font-weight: normal;
	}

	/* 指标卡片 */
	.metric-grid {
		display: flex;
		flex-direction: row;
		flex-wrap: wrap;
		justify-content: space-between;
	}

	.metric-card {
		position: relative;
		width: 48.2%;
		border-radius: 18rpx;
		padding: 24rpx 20rpx;
		margin-bottom: 20rpx;
		box-sizing: border-box;
		animation: card-in 0.45s ease both;
		transition: transform 0.18s ease, box-shadow 0.18s ease;
	}

	.metric-card:active {
		transform: scale(0.96);
	}

	.metric-icon {
		width: 56rpx;
		height: 56rpx;
		border-radius: 14rpx;
		display: flex;
		align-items: center;
		justify-content: center;
		margin-bottom: 12rpx;
	}

	/* 双温度卡：与普通卡同宽，内部左右挤两路温度，顶部对齐 */
	.dual-temp-card {
		display: flex;
		flex-direction: row;
		align-items: stretch;
	}

	.temp-half {
		flex: 1;
		min-width: 0;
		padding: 0 6rpx;
		box-sizing: border-box;
		display: flex;
		flex-direction: column;
		justify-content: center;
	}

	.temp-half .metric-label {
		font-size: 22rpx;
		white-space: nowrap;
	}

	.temp-head {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.metric-icon.sm {
		width: 32rpx;
		height: 32rpx;
		border-radius: 8rpx;
		margin-bottom: 0;
		margin-right: 6rpx;
	}

	.metric-icon.sm .icon-text {
		font-size: 18rpx;
	}

	.temp-value {
		font-size: 34rpx;
		font-weight: 600;
		line-height: 1.1;
		color: #1f2937;
	}

	.temp-divider {
		width: 2rpx;
		align-self: stretch;
		margin: 4rpx 2rpx;
		background: #eef1f5;
	}

	.icon-text {
		font-size: 28rpx;
		color: #ffffff;
	}

	.metric-label {
		font-size: 24rpx;
		color: #6b7280;
		line-height: 1.2;
	}

	.metric-value-row {
		display: flex;
		flex-direction: row;
		align-items: baseline;
		margin-top: 6rpx;
	}

	.metric-value {
		font-size: 42rpx;
		font-weight: 600;
		line-height: 1.1;
		color: #1f2937;
	}

	.metric-unit {
		font-size: 22rpx;
		color: #9ca3af;
		margin-left: 8rpx;
	}

	.metric-link {
		position: absolute;
		top: 22rpx;
		right: 20rpx;
		font-size: 20rpx;
		color: #2b7fff;
	}

	/* 趋势图 */
	.trend-card {
		border-radius: 18rpx;
		padding: 24rpx 20rpx 20rpx;
		animation: card-in 0.5s 0.12s ease both;
	}

	.trend-header {
		display: flex;
		justify-content: space-between;
		align-items: flex-start;
		padding: 0 4rpx;
	}

	.trend-name {
		font-size: 28rpx;
		font-weight: 600;
		transition: color 0.25s;
	}

	.trend-desc {
		display: block;
		font-size: 22rpx;
		color: #9ca3af;
		margin-top: 6rpx;
	}

	.trend-now {
		display: flex;
		flex-direction: row;
		align-items: baseline;
	}

	.trend-value {
		font-size: 38rpx;
		font-weight: 600;
		transition: color 0.25s;
	}

	.trend-unit {
		font-size: 22rpx;
		color: #9ca3af;
		margin-left: 6rpx;
	}

	.trend-swiper {
		height: 240rpx;
		margin-top: 8rpx;
	}

	.chart-wrap {
		position: relative;
		height: 100%;
		padding: 0 4rpx;
		box-sizing: border-box;
	}

	.trend-canvas {
		width: 100%;
		height: 240rpx;
	}

	/* 覆盖在 canvas 上的透明触摸层，尺寸与 canvas 绘图区对齐 */
	.chart-touch-layer {
		position: absolute;
		left: 4rpx;
		right: 4rpx;
		top: 0;
		height: 240rpx;
		z-index: 5;
	}

	.chart-empty {
		position: absolute;
		left: 0;
		right: 0;
		top: 0;
		height: 240rpx;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.chart-empty text {
		font-size: 24rpx;
		color: #b0b6bf;
	}

	.axis {
		display: flex;
		justify-content: center;
		padding: 8rpx 10rpx 0;
	}

	.axis.three {
		justify-content: space-between;
	}

	.chart-tap-hint {
		display: block;
		text-align: center;
		font-size: 19rpx;
		color: #b0b6bf;
		padding-top: 6rpx;
	}

	.axis-text {
		font-size: 19rpx;
		color: #9ca3af;
	}

	.dots {
		display: flex;
		flex-direction: row;
		justify-content: center;
		margin-top: 12rpx;
	}

	.dot {
		width: 12rpx;
		height: 12rpx;
		border-radius: 6rpx;
		background: #d1d5db;
		margin: 0 6rpx;
		transition: all 0.25s cubic-bezier(0.34, 1.4, 0.64, 1);
	}

	.dot.active {
		width: 30rpx;
	}

	.trend-range {
		display: flex;
		justify-content: space-between;
		margin-top: 12rpx;
		padding: 14rpx 10rpx 0;
		border-top: 2rpx solid #f1f3f6;
	}

	.range-text {
		font-size: 22rpx;
		color: #9ca3af;
	}
</style>
