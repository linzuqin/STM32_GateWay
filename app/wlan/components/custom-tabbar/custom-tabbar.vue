<template>
	<view class="tabbar-wrap">
		<view class="tabbar">
			<view
				v-for="(item, i) in items"
				:key="item.key"
				class="tab-item"
				@click="tap(i)"
			>
				<!-- 高亮背景块：位于每项内部最底层，结构上不可能挡住图标/文字 -->
				<view class="item-bg" :class="{ active: current === i }"></view>
				<view class="tab-icon-box" :class="{ active: current === i }">
					<text class="tab-icon" :class="{ active: current === i }">{{ item.icon }}</text>
				</view>
				<text class="tab-label" :class="{ active: current === i }">{{ item.label }}</text>
			</view>
		</view>
	</view>
</template>

<script>
	export default {
		name: 'CustomTabbar',
		props: {
			current: {
				type: Number,
				default: 0
			}
		},
		data() {
			return {
				items: [
					{ key: 'home', label: '首页', icon: '⌂' },
					{ key: 'settings', label: '设置', icon: '⚙' },
					{ key: 'device', label: '设备详情', icon: '◉' },
					{ key: 'msh', label: 'MSH', icon: '令' },
					{ key: 'account', label: '账户', icon: '我' }
				]
			}
		},
		methods: {
			tap(i) {
				if (i === this.current) return
				uni.vibrateShort && uni.vibrateShort({ type: 'light' })
				this.$emit('change', i)
			}
		}
	}
</script>

<style>
	.tabbar-wrap {
		position: fixed;
		left: 0;
		right: 0;
		bottom: 0;
		z-index: 999;
		padding: 0 20rpx;
		padding-bottom: constant(safe-area-inset-bottom);
		padding-bottom: env(safe-area-inset-bottom);
		box-sizing: border-box;
		pointer-events: none;
	}

	.tabbar {
		pointer-events: auto;
		position: relative;
		display: flex;
		flex-direction: row;
		height: 116rpx;
		background: rgba(255, 255, 255, 0.92);
		backdrop-filter: blur(20px);
		border-radius: 32rpx;
		box-shadow: 0 8rpx 40rpx rgba(31, 41, 55, 0.12);
		margin-bottom: 16rpx;
	}

	.tab-item {
		flex: 1;
		position: relative;
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
	}

	/* 高亮背景块：在 item 内部铺满（留 12rpx 边距），位于内容之下 */
	.item-bg {
		position: absolute;
		left: 12rpx;
		right: 12rpx;
		top: 12rpx;
		bottom: 12rpx;
		border-radius: 24rpx;
		background: linear-gradient(135deg, #2b7fff 0%, #4fa3ff 100%);
		box-shadow: 0 8rpx 18rpx rgba(43, 127, 255, 0.32);
		opacity: 0;
		transform: scale(0.72);
		transition: opacity 0.3s ease, transform 0.38s cubic-bezier(0.34, 1.4, 0.5, 1);
	}

	.item-bg.active {
		opacity: 1;
		transform: scale(1);
	}

	/* 图标与文字在背景块之上 */
	.tab-icon-box {
		position: relative;
		z-index: 2;
		width: 64rpx;
		height: 46rpx;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: transform 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
	}

	.tab-icon-box.active {
		animation: icon-pop 0.4s cubic-bezier(0.34, 1.56, 0.64, 1);
	}

	@keyframes icon-pop {
		0% { transform: scale(0.6) translateY(6rpx); }
		60% { transform: scale(1.18) translateY(-4rpx); }
		100% { transform: scale(1) translateY(0); }
	}

	.tab-icon {
		font-size: 38rpx;
		color: #9ca3af;
		line-height: 1;
		transition: color 0.3s;
	}

	.tab-icon.active {
		color: #ffffff;
	}

	.tab-label {
		position: relative;
		z-index: 2;
		font-size: 20rpx;
		color: #9ca3af;
		margin-top: 4rpx;
		transition: color 0.3s;
	}

	.tab-label.active {
		color: #ffffff;
		font-weight: 600;
	}
</style>
