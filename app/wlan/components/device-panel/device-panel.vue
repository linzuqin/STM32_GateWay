<template>
	<view class="page">
		<!-- 设备概要 -->
		<view class="device-card">
			<view class="device-top">
				<view class="device-id">
					<text class="device-name">环境监测</text>
					<view class="status-tag" :class="onlineClass">
						<view class="status-dot"></view>
						<text class="status-text">{{ onlineText }}</text>
					</view>
				</view>
			</view>
			<view class="device-info">
				<view class="info-row">
					<text class="info-label">产品ID</text>
					<text class="info-value mono">{{ config.PRODUCT_ID }}</text>
				</view>
				<view class="info-row">
					<text class="info-label">设备名称</text>
					<text class="info-value mono">{{ config.DEVICE_NAME }}</text>
				</view>
				<view class="info-row">
					<text class="info-label">最新上报</text>
					<text class="info-value">{{ state.reportTime || '--' }}</text>
				</view>
			</view>
		</view>

		<!-- 设备属性（物模型 properties） -->
		<view class="section-title">
			<text class="title-bar"></text>
			<text class="title-text">设备属性</text>
			<text class="title-extra">{{ propList.length }} 个功能点</text>
		</view>
		<view class="card prop-card">
			<view
				v-for="(p, i) in propList"
				:key="p.identifier"
				class="prop-row"
				:class="{ 'no-border': i === propList.length - 1 }"
			>
				<view class="prop-left">
					<text class="prop-name">{{ p.name }}</text>
					<text class="prop-meta mono">{{ p.identifier }} · {{ p.dataType }}</text>
				</view>
				<text class="prop-value" :class="{ num: p.metric }">{{ displayValue(p) }}</text>
			</view>
		</view>

		<view class="footer-tip">
			<text>数据来源：query-device-property（OneNet doc/1421）</text>
		</view>
	</view>
</template>

<script>
	import { state, PROP_META, formatNum, bitmapBitOn, activeAlarms } from '@/utils/onenet/store.js'
	import { ONENET_CONFIG } from '@/utils/onenet/config.js'

	export default {
		name: 'DevicePanel',
		data() {
			return {
				state,
				config: ONENET_CONFIG
			}
		},
		computed: {
			propList() {
				return PROP_META
			},
			onlineClass() {
				return state.online === null ? 'unknown' : (state.online ? 'on' : 'off')
			},
			onlineText() {
				return state.online === null ? '未知' : (state.online ? '在线' : '离线')
			}
		},
		methods: {
			displayValue(p) {
				const v = state.propsData[p.identifier]
				if (v === undefined || v === null || v === '') return '--'
				// 位图：逐位显示，如「继电器1 ON · 继电器2 OFF」
				if (p.dataType === 'bitMap' && p.bits) {
					if (p.identifier === 'alarmFlag') {
						const list = activeAlarms(v)
						return list.length ? list.join(' · ') + '（值 ' + v + '）' : '正常（值 0）'
					}
					return p.bits.map(b => {
						return b.name + ' ' + (bitmapBitOn(v, b.bit) ? 'ON' : 'OFF')
					}).join(' · ')
				}
				// 枚举：映射中文名，未知值附原始值
				if (p.dataType === 'enum' && p.enumMap) {
					return p.enumMap[v] !== undefined ? p.enumMap[v] + '（值 ' + v + '）' : '未知（值 ' + v + '）'
				}
				// 阈值类数值带单位
				if (p.unit && (p.metric || p.setting)) {
					return formatNum(v, p.decimals === undefined ? 0 : p.decimals) + ' ' + p.unit
				}
				return p.metric ? formatNum(v, p.decimals) + ' ' + p.unit : v
			}
		}
	}
</script>

<style>
	.device-card {
		border-radius: 20rpx;
		padding: 32rpx 28rpx 28rpx;
		background: linear-gradient(135deg, #2b7fff 0%, #4fa3ff 100%);
		box-shadow: 0 12rpx 30rpx rgba(43, 127, 255, 0.28);
		animation: card-in 0.45s ease both;
	}

	@keyframes card-in {
		from { opacity: 0; transform: translateY(24rpx); }
		to { opacity: 1; transform: translateY(0); }
	}

	.device-top {
		display: flex;
		justify-content: space-between;
		align-items: center;
	}

	.device-id {
		display: flex;
		flex-direction: column;
	}

	.device-name {
		font-size: 36rpx;
		font-weight: 600;
		color: #ffffff;
	}

	.status-tag {
		display: flex;
		flex-direction: row;
		align-items: center;
		margin-top: 10rpx;
		border-radius: 100rpx;
		padding: 4rpx 16rpx;
		align-self: flex-start;
		background: rgba(255, 255, 255, 0.18);
	}

	.status-tag .status-dot {
		width: 12rpx;
		height: 12rpx;
		border-radius: 50%;
		margin-right: 8rpx;
		background: #9ca3af;
	}

	.status-tag.on .status-dot {
		background: #4ade80;
		animation: pulse 1.6s infinite;
	}

	.status-tag.off .status-dot {
		background: #f87171;
	}

	.status-text {
		font-size: 22rpx;
		color: #ffffff;
	}

	@keyframes pulse {
		0% { opacity: 1; }
		50% { opacity: 0.3; }
		100% { opacity: 1; }
	}

	.device-info {
		margin-top: 24rpx;
		background: rgba(255, 255, 255, 0.14);
		border-radius: 14rpx;
		padding: 12rpx 20rpx;
	}

	.info-row {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding: 8rpx 0;
	}

	.info-label {
		font-size: 24rpx;
		color: rgba(255, 255, 255, 0.85);
	}

	.info-value {
		font-size: 24rpx;
		color: #ffffff;
	}

	.title-extra {
		font-size: 20rpx;
		color: #9ca3af;
		margin-left: auto;
		font-weight: normal;
	}

	.prop-card {
		padding: 0 24rpx;
		animation: card-in 0.45s 0.08s ease both;
	}

	.prop-row {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding: 24rpx 0;
		border-bottom: 2rpx solid #f1f3f6;
	}

	.prop-row.no-border {
		border-bottom: none;
	}

	.prop-left {
		display: flex;
		flex-direction: column;
	}

	.prop-name {
		font-size: 28rpx;
		color: #1f2937;
		line-height: 1.2;
	}

	.prop-meta {
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 6rpx;
	}

	.prop-value {
		font-size: 26rpx;
		color: #374151;
		max-width: 320rpx;
		text-align: right;
	}

	.prop-value.num {
		color: #111827;
		font-weight: 600;
	}
</style>
