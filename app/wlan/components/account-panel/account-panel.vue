<template>
	<view class="page">
		<!-- 用户头部（未登录态；后续接入登录后替换为真实用户信息） -->
		<view class="user-card">
			<view class="avatar">
				<text class="avatar-text">人</text>
			</view>
			<view class="user-info">
				<text class="user-name">{{ isLoggedIn ? userName : '未登录' }}</text>
				<text class="user-desc">{{ isLoggedIn ? '欢迎回来' : '登录后可同步设备与告警设置' }}</text>
			</view>
			<view v-if="!isLoggedIn" class="login-btn" hover-class="btn-hover" @click="onLogin">
				<text class="login-btn-text">登录</text>
			</view>
		</view>

		<!-- 当前设备连接状态 -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">设备连接</text>
		</view>
		<view class="card conn-card">
			<view class="conn-row">
				<text class="conn-label">产品 ID</text>
				<text class="conn-value">{{ productId }}</text>
			</view>
			<view class="conn-row">
				<text class="conn-label">设备名称</text>
				<text class="conn-value">{{ deviceName }}</text>
			</view>
			<view class="conn-row">
				<text class="conn-label">连接状态</text>
				<view class="conn-status">
					<text class="status-dot" :class="onlineClass"></text>
					<text class="conn-value" :class="onlineClass">{{ onlineText }}</text>
				</view>
			</view>
			<view class="conn-row no-border">
				<text class="conn-label">最新上报</text>
				<text class="conn-value">{{ state.reportTime || '暂无数据' }}</text>
			</view>
		</view>

		<!-- 预留功能菜单 -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">账户</text>
		</view>
		<view class="card menu-card">
			<view
				v-for="(m, i) in menus"
				:key="m.key"
				class="menu-row"
				:class="{ 'no-border': i === menus.length - 1, disabled: !m.enabled }"
				@click="onMenu(m)"
			>
				<view class="menu-left">
					<view class="menu-icon" :style="{ background: m.color }">
						<text class="menu-icon-text">{{ m.icon }}</text>
					</view>
					<text class="menu-name">{{ m.name }}</text>
				</view>
				<view class="menu-right">
					<text v-if="m.tip" class="menu-tip">{{ m.tip }}</text>
					<text class="menu-arrow">›</text>
				</view>
			</view>
		</view>

		<!-- 预留：登录后才显示的操作 -->
		<template v-if="isLoggedIn">
			<view class="section-title">
				<text class="title-bar"></text>
				<text class="title-text">其他</text>
			</view>
			<view class="card menu-card">
				<view class="menu-row no-border" @click="onLogout">
					<view class="menu-left">
						<view class="menu-icon" style="background: #f87171">
							<text class="menu-icon-text">退</text>
						</view>
						<text class="menu-name">退出登录</text>
					</view>
					<text class="menu-arrow">›</text>
				</view>
			</view>
		</template>

		<view class="footer-tip">
			<text>账户功能开发中 · v0.1.0</text>
		</view>
	</view>
</template>

<script>
	import { state } from '@/utils/onenet/store.js'
	import { ONENET_CONFIG } from '@/utils/onenet/config.js'

	export default {
		name: 'AccountPanel',
		data() {
			return {
				// 预留登录态：后续对接登录接口后，由全局 store/storage 驱动
				isLoggedIn: false,
				userName: '',
				menus: [
					{ key: 'profile', name: '个人资料', icon: '资', color: '#2b7fff', enabled: false, tip: '即将上线' },
					{ key: 'security', name: '账号安全', icon: '安', color: '#ffa94d', enabled: false, tip: '即将上线' },
					{ key: 'notify', name: '消息通知', icon: '讯', color: '#38d9a9', enabled: false, tip: '即将上线' },
					{ key: 'device', name: '我的设备', icon: '机', color: '#4dabf7', enabled: false, tip: '即将上线' },
					{ key: 'cache', name: '清除缓存', icon: '缓', color: '#ffa94d', enabled: true },
					{ key: 'update', name: '检查更新', icon: '新', color: '#38d9a9', enabled: true, tip: 'v0.1.0' },
					{ key: 'about', name: '关于应用', icon: '关', color: '#9ca3af', enabled: true }
				]
			}
		},
		computed: {
			state() {
				return state
			},
			productId() {
				return ONENET_CONFIG.PRODUCT_ID
			},
			deviceName() {
				return ONENET_CONFIG.DEVICE_NAME
			},
			onlineText() {
				if (state.online === null) return '连接中'
				return state.online ? '在线' : '离线'
			},
			onlineClass() {
				if (state.online === null) return 'idle'
				return state.online ? 'online' : 'offline'
			}
		},
		methods: {
			onLogin() {
				// TODO: 后续跳转独立登录页（账号密码 / 短信验证码）
				uni.showToast({ title: '登录功能即将上线', icon: 'none' })
			},
			onLogout() {
				this.isLoggedIn = false
				this.userName = ''
			},
			onMenu(m) {
				if (!m.enabled) {
					uni.showToast({ title: (m.tip || '功能') + '，敬请期待', icon: 'none' })
					return
				}
				if (m.key === 'about') {
					uni.showModal({
						title: 'OneNet 设备监控',
						content: '版本 v0.1.0\n用于环境监测开发板的数据展示与风扇控制',
						showCancel: false
					})
					return
				}
				if (m.key === 'update') {
					uni.showToast({ title: '当前已是最新版本 v0.1.0', icon: 'none' })
					return
				}
				if (m.key === 'cache') {
					uni.showModal({
						title: '清除缓存',
						content: '将清除本机缓存数据，不影响设备配置。确定清除吗？',
						confirmText: '清除',
						success: (res) => {
							if (res.confirm) {
								try {
									uni.clearStorageSync()
									uni.showToast({ title: '缓存已清除', icon: 'success' })
								} catch (e) {
									uni.showToast({ title: '清除失败', icon: 'none' })
								}
							}
						}
					})
				}
			}
		}
	}
</script>

<style>
	.user-card {
		display: flex;
		flex-direction: row;
		align-items: center;
		border-radius: 20rpx;
		padding: 36rpx 28rpx;
		background: linear-gradient(135deg, #2b7fff 0%, #4fa3ff 100%);
		box-shadow: 0 12rpx 30rpx rgba(43, 127, 255, 0.28);
		animation: card-in 0.45s ease both;
	}

	@keyframes card-in {
		from { opacity: 0; transform: translateY(24rpx); }
		to { opacity: 1; transform: translateY(0); }
	}

	.avatar {
		width: 112rpx;
		height: 112rpx;
		border-radius: 50%;
		background: rgba(255, 255, 255, 0.22);
		border: 4rpx solid rgba(255, 255, 255, 0.55);
		display: flex;
		align-items: center;
		justify-content: center;
		flex-shrink: 0;
	}

	.avatar-text {
		font-size: 48rpx;
		color: #ffffff;
	}

	.user-info {
		flex: 1;
		display: flex;
		flex-direction: column;
		margin-left: 24rpx;
		min-width: 0;
	}

	.user-name {
		font-size: 36rpx;
		font-weight: 600;
		color: #ffffff;
	}

	.user-desc {
		font-size: 22rpx;
		color: rgba(255, 255, 255, 0.85);
		margin-top: 10rpx;
	}

	.login-btn {
		flex-shrink: 0;
		padding: 0 32rpx;
		height: 64rpx;
		border-radius: 100rpx;
		background: #ffffff;
		display: flex;
		align-items: center;
		justify-content: center;
		box-shadow: 0 6rpx 16rpx rgba(0, 0, 0, 0.12);
	}

	.btn-hover {
		transform: scale(0.95);
	}

	.login-btn-text {
		font-size: 26rpx;
		color: #2b7fff;
		font-weight: 600;
	}

	.conn-card {
		padding: 4rpx 24rpx;
		animation: card-in 0.45s 0.05s ease both;
	}

	.conn-row {
		display: flex;
		flex-direction: row;
		justify-content: space-between;
		align-items: center;
		padding: 22rpx 0;
		border-bottom: 2rpx solid #f1f3f6;
	}

	.conn-row.no-border {
		border-bottom: none;
	}

	.conn-label {
		font-size: 26rpx;
		color: #8a9099;
	}

	.conn-value {
		font-size: 26rpx;
		color: #1f2937;
		max-width: 420rpx;
		text-align: right;
	}

	.conn-status {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.status-dot {
		width: 14rpx;
		height: 14rpx;
		border-radius: 50%;
		margin-right: 10rpx;
	}

	.status-dot.online {
		background: #22c55e;
		box-shadow: 0 0 0 6rpx rgba(34, 197, 94, 0.15);
	}

	.status-dot.offline {
		background: #ef4444;
		box-shadow: 0 0 0 6rpx rgba(239, 68, 68, 0.12);
	}

	.status-dot.idle {
		background: #f59e0b;
		box-shadow: 0 0 0 6rpx rgba(245, 158, 11, 0.14);
	}

	.conn-value.online {
		color: #16a34a;
	}

	.conn-value.offline {
		color: #dc2626;
	}

	.conn-value.idle {
		color: #d97706;
	}

	.menu-card {
		padding: 0 24rpx;
		animation: card-in 0.45s 0.1s ease both;
	}

	.menu-row {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding: 22rpx 0;
		border-bottom: 2rpx solid #f1f3f6;
	}

	.menu-row.no-border {
		border-bottom: none;
	}

	.menu-left {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.menu-icon {
		width: 60rpx;
		height: 60rpx;
		border-radius: 14rpx;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.menu-icon-text {
		font-size: 28rpx;
		color: #ffffff;
	}

	.menu-name {
		font-size: 28rpx;
		color: #1f2937;
		margin-left: 20rpx;
	}

	.menu-right {
		display: flex;
		flex-direction: row;
		align-items: center;
	}

	.menu-tip {
		font-size: 22rpx;
		color: #b0b6bf;
		margin-right: 8rpx;
	}

	.menu-arrow {
		font-size: 36rpx;
		color: #c8cdd4;
		line-height: 1;
	}

	.menu-row.disabled {
		opacity: 0.75;
	}
</style>
