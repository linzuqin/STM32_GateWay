<template>
	<view class="page">
		<!-- 顶部说明卡 -->
		<view class="card msh-head-card">
			<view class="msh-head">
				<view class="msh-head-left">
					<text class="msh-title">MSH 指令</text>
					<text class="msh-sub">msh · string · 写入属性由设备解析执行</text>
				</view>
				<view class="status-pill" :class="onlineClass">
					<view class="status-dot"></view>
					<text class="status-text">{{ onlineText }}</text>
				</view>
			</view>
			<text class="msh-desc">点击快捷指令立即下发；reboot 与 clean_flash 需二次确认。</text>
		</view>

		<!-- 快捷指令 -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">快捷指令</text>
		</view>
		<view class="card cmd-grid">
			<view
				v-for="c in commands"
				:key="c.cmd"
				class="cmd-item"
				:class="[c.tone, { disabled: state.online === false || state.setting }]"
				hover-class="cmd-hover"
				@click="tapCommand(c)"
			>
				<text class="cmd-name">{{ c.name }}</text>
				<text class="cmd-cmd mono">{{ c.cmd }}</text>
			</view>
		</view>

		<!-- set_ip -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">设置 IP</text>
		</view>
		<view class="card ip-card">
			<view class="ip-row">
				<input
					class="ip-input mono"
					v-model="ipValue"
					placeholder="xxx.xxx.xxx.xxx:port"
					placeholder-class="ip-ph"
				/>
				<view
					class="ip-btn"
					:class="{ disabled: state.online === false || state.setting || !ipValid }"
					hover-class="btn-hover"
					@click="submitIp"
				>
					<text class="ip-btn-text">{{ state.setting ? '下发中' : '下发' }}</text>
				</view>
			</view>
			<text class="ip-tip" :class="{ bad: ipValue && !ipValid }">
				{{ ipValue && !ipValid ? '格式不正确，应为 xxx.xxx.xxx.xxx:端口（0-65535）' : '下发命令：set_ip ' + (ipValue || 'xxx.xxx.xxx.xxx:port') }}
			</text>
		</view>

		<!-- 自定义指令 -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">自定义指令</text>
		</view>
		<view class="card custom-card">
			<view class="ip-row">
				<input
					class="ip-input mono"
					v-model="customCmd"
					placeholder="输入任意 msh 指令，如 help"
					placeholder-class="ip-ph"
					confirm-type="send"
					@confirm="submitCustom"
				/>
				<view
					class="ip-btn custom"
					:class="{ disabled: state.online === false || state.setting || !customCmd.trim() }"
					hover-class="btn-hover"
					@click="submitCustom"
				>
					<text class="ip-btn-text">发送</text>
				</view>
			</view>
		</view>

		<!-- 下发记录 -->
		<view class="section-title hist-title">
			<text class="title-bar"></text>
			<text class="title-text">下发记录</text>
			<text v-if="history.length" class="title-extra" @click="clearHistory">清空</text>
		</view>
		<view class="card hist-card">
			<view v-if="!history.length" class="hist-empty">
				<text>暂无下发记录</text>
			</view>
			<view
				v-for="(h, i) in history"
				:key="i"
				class="hist-row"
				:class="{ 'no-border': i === history.length - 1 }"
			>
				<view class="hist-dot" :class="h.ok ? 'ok' : 'fail'"></view>
				<view class="hist-body">
					<text class="hist-cmd mono">{{ h.cmd }}</text>
					<text class="hist-time">{{ h.time }}<text v-if="!h.ok" class="hist-err"> · {{ h.msg }}</text></text>
				</view>
				<text class="hist-status" :class="h.ok ? 'ok' : 'fail'">{{ h.ok ? '成功' : '失败' }}</text>
			</view>
		</view>

		<view class="footer-tip">
			<text>通过 set-device-property 写入 msh 字符串属性（OneNet doc/1418）</text>
		</view>
	</view>
</template>

<script>
	import store, { state } from '@/utils/onenet/store.js'

	export default {
		name: 'MshPanel',
		data() {
			return {
				store,
				state,
				ipValue: '',
				customCmd: '',
				// 本次运行内的下发记录
				history: [],
				commands: [
					{ cmd: 'reboot', name: '设备重启', tone: 'danger', confirm: true },
					{ cmd: 'test', name: '测试模式', tone: 'blue', confirm: false },
					{ cmd: 'clean_flash', name: '清除 Flash', tone: 'danger', confirm: true },
					{ cmd: 'otaRun', name: '远程升级', tone: 'green', confirm: false }
				]
			}
		},
		computed: {
			onlineClass() {
				return state.online === null ? 'unknown' : (state.online ? 'on' : 'off')
			},
			onlineText() {
				return state.online === null ? '未知' : (state.online ? '在线' : '离线')
			},
			// 校验 xxx.xxx.xxx.xxx:port
			ipValid() {
				const v = (this.ipValue || '').trim()
				const m = v.match(/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}):(\d{1,5})$/)
				if (!m) return false
				for (let i = 1; i <= 4; i++) {
					if (Number(m[i]) > 255) return false
				}
				const port = Number(m[5])
				return port >= 1 && port <= 65535
			}
		},
		methods: {
			tapCommand(c) {
				if (state.online === false || state.setting) {
					if (state.online === false) uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					return
				}
				if (c.confirm) {
					uni.showModal({
						title: c.name,
						content: '确认下发「' + c.cmd + '」？' + (c.cmd === 'reboot' ? '设备将立即重启。' : 'Flash 存储数据将被清除。'),
						confirmColor: '#ef4444',
						success: (res) => {
							if (res.confirm) this.send(c.cmd)
						}
					})
				} else {
					this.send(c.cmd)
				}
			},
			submitIp() {
				if (state.online === false || state.setting) {
					if (state.online === false) uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					return
				}
				if (!this.ipValid) {
					uni.showToast({ title: 'IP 格式不正确', icon: 'none' })
					return
				}
				this.send('set_ip ' + this.ipValue.trim())
			},
			submitCustom() {
				if (state.online === false || state.setting) {
					if (state.online === false) uni.showToast({ title: '设备离线，无法下发', icon: 'none' })
					return
				}
				const cmd = this.customCmd.trim()
				if (!cmd) return
				this.send(cmd, () => { this.customCmd = '' })
			},
			// 统一下发并记录
			send(cmd, afterOk) {
				store.sendMsh(cmd).then(() => {
					this.pushHistory(cmd, true)
					uni.showToast({ title: '指令已下发', icon: 'none' })
					if (afterOk) afterOk()
				}).catch(err => {
					console.error('[OneNet] set-device-property(msh) 失败', err)
					this.pushHistory(cmd, false, err.msg || '下发失败')
					uni.showToast({ title: err.msg || '下发失败', icon: 'none' })
				})
			},
			pushHistory(cmd, ok, msg) {
				const d = new Date()
				const pad = n => (n < 10 ? '0' + n : '' + n)
				const time = pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds())
				this.history.unshift({ cmd, ok, msg: msg || '', time })
				if (this.history.length > 30) this.history.pop()
			},
			clearHistory() {
				this.history = []
			}
		}
	}
</script>

<style>
	.msh-head-card {
		padding: 32rpx 28rpx 28rpx;
	}

	.msh-head {
		display: flex;
		justify-content: space-between;
		align-items: flex-start;
	}

	.msh-head-left {
		display: flex;
		flex-direction: column;
	}

	.msh-title {
		font-size: 32rpx;
		font-weight: 600;
		color: #1f2937;
	}

	.msh-sub {
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 8rpx;
	}

	.msh-desc {
		display: block;
		font-size: 22rpx;
		color: #9ca3af;
		margin-top: 18rpx;
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

	/* 快捷指令 */
	.cmd-grid {
		display: flex;
		flex-wrap: wrap;
		justify-content: space-between;
		padding: 24rpx;
	}

	.cmd-item {
		width: 48%;
		box-sizing: border-box;
		border-radius: 16rpx;
		padding: 26rpx 22rpx;
		margin-bottom: 20rpx;
		border: 2rpx solid transparent;
		transition: transform 0.15s ease, opacity 0.2s;
	}

	.cmd-item:nth-last-child(-n+2) {
		margin-bottom: 0;
	}

	.cmd-item:active {
		transform: scale(0.96);
	}

	.cmd-item.disabled {
		opacity: 0.5;
	}

	.cmd-item.danger {
		background: #fff1f2;
		border-color: #fecdd3;
	}

	.cmd-item.blue {
		background: #eff6ff;
		border-color: #bfdbfe;
	}

	.cmd-item.green {
		background: #ecfdf5;
		border-color: #a7f3d0;
	}

	.cmd-name {
		font-size: 28rpx;
		font-weight: 600;
	}

	.cmd-item.danger .cmd-name {
		color: #e11d48;
	}

	.cmd-item.blue .cmd-name {
		color: #2563eb;
	}

	.cmd-item.green .cmd-name {
		color: #059669;
	}

	.cmd-cmd {
		display: block;
		font-size: 22rpx;
		color: #9ca3af;
		margin-top: 10rpx;
	}

	/* IP / 自定义 */
	.ip-card,
	.custom-card {
		padding: 24rpx;
	}

	.ip-row {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.ip-input {
		flex: 1;
		height: 76rpx;
		background: #f9fafb;
		border-radius: 12rpx;
		padding: 0 20rpx;
		font-size: 27rpx;
		color: #111827;
	}

	.ip-ph {
		color: #b0b6bf;
		font-size: 25rpx;
	}

	.ip-btn {
		margin-left: 16rpx;
		padding: 0 34rpx;
		height: 76rpx;
		border-radius: 12rpx;
		background: linear-gradient(135deg, #2b7fff 0%, #4fa3ff 100%);
		display: flex;
		align-items: center;
		justify-content: center;
		box-shadow: 0 6rpx 16rpx rgba(43, 127, 255, 0.25);
	}

	.ip-btn.custom {
		background: linear-gradient(135deg, #7c3aed 0%, #9775fa 100%);
		box-shadow: 0 6rpx 16rpx rgba(124, 58, 237, 0.25);
	}

	.ip-btn.disabled {
		opacity: 0.5;
		box-shadow: none;
	}

	.ip-btn-text {
		font-size: 27rpx;
		color: #ffffff;
		font-weight: 500;
	}

	.btn-hover {
		opacity: 0.85;
	}

	.ip-tip {
		display: block;
		font-size: 21rpx;
		color: #9ca3af;
		margin-top: 14rpx;
	}

	.ip-tip.bad {
		color: #ef4444;
	}

	/* 记录 */
	.hist-title .title-extra {
		font-size: 22rpx;
		color: #2b7fff;
		margin-left: auto;
	}

	.hist-card {
		padding: 0 24rpx;
	}

	.hist-empty {
		padding: 44rpx 0;
		text-align: center;
		font-size: 24rpx;
		color: #b0b6bf;
	}

	.hist-row {
		display: flex;
		flex-direction: row;
		align-items: center;
		padding: 22rpx 0;
		border-bottom: 2rpx solid #f1f3f6;
	}

	.hist-row.no-border {
		border-bottom: none;
	}

	.hist-dot {
		width: 14rpx;
		height: 14rpx;
		border-radius: 50%;
		margin-right: 16rpx;
		flex-shrink: 0;
	}

	.hist-dot.ok {
		background: #22c55e;
	}

	.hist-dot.fail {
		background: #ef4444;
	}

	.hist-body {
		flex: 1;
		display: flex;
		flex-direction: column;
		min-width: 0;
	}

	.hist-cmd {
		font-size: 26rpx;
		color: #1f2937;
	}

	.hist-time {
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 6rpx;
	}

	.hist-err {
		color: #ef4444;
	}

	.hist-status {
		font-size: 22rpx;
		margin-left: 12rpx;
	}

	.hist-status.ok {
		color: #16a34a;
	}

	.hist-status.fail {
		color: #dc2626;
	}
</style>
